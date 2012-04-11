/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "WrappingContext.h"

#include "RenderBlock.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "RootInlineBox.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

PassOwnPtr<WrappingContext> WrappingContext::create(RenderLayer* layer)
{
    return adoptPtr(new WrappingContext(layer));
}

WrappingContext::WrappingContext(RenderLayer* layer)
    : m_layer(layer)
    , m_parent(0)
    , m_hasDirtyChildContextOrder(false)
{
}

WrappingContext::~WrappingContext()
{
    ASSERT(!m_parent);
}

void WrappingContext::didAddOnlyThisContext()
{
    ASSERT(!m_parent);
    WrappingContext* parentContext = layer()->enclosingWrappingContext(false);
    m_parent = parentContext;
    if (m_parent) {
        // Steal all the child contexts from the parent if they are actually ours.
        for (size_t i = 0; i < m_parent->childCount(); ++i) {
            WrappingContext* child = m_parent->childAt(i);
            if (child->layer()->enclosingWrappingContext(false) == this) {
                child->setParent(this);
                addChildContext(child);
            }
        }
        for (size_t i = 0; i < childCount(); ++i)
            m_parent->removeChildContext(childAt(i));
        
        m_parent->addChildContext(this);
    }
}

void WrappingContext::willRemoveOnlyThisContext()
{
    ASSERT(m_parent || !childCount());
    if (!m_parent)
        return;
    for (size_t i = 0; i < childCount(); ++i) {
        WrappingContext* child = childAt(i);
        child->setParent(m_parent);
        m_parent->addChildContext(child);
    }
    m_parent->removeChildContext(this);
    m_parent = 0;
}

void WrappingContext::didAddLayer()
{
    WrappingContext* parentContext = layer()->enclosingWrappingContext(false);
    m_parent = parentContext;
    if (m_parent)
        m_parent->addChildContext(this);
}

void WrappingContext::willRemoveLayer()
{
    if (!m_parent)
        return;
    m_parent->removeChildContext(this);
    m_parent = 0;
}

void WrappingContext::addChildContext(WrappingContext* child)
{
    m_children.append(child);
    setHasDirtyChildContextsOrder();
}

void WrappingContext::removeChildContext(WrappingContext* child)
{
    size_t index = m_children.find(child);
    if (index == notFound) {
        ASSERT_NOT_REACHED();
        return;
    }
    m_children.remove(index);
    setHasDirtyChildContextsOrder();
}

static bool findLayerInList(Vector<RenderLayer*>* list, RenderLayer* layer, size_t& position)
{
    if (!list)
        return false;
    size_t positionInList = list->find(layer);
    if (positionInList != notFound) {
        position += positionInList;
        return true;
    }
    position += list->size();
    return false;
}

static size_t layerPositionInStackingContext(RenderLayer* stackingContext, RenderLayer* layer)
{
    size_t position = 0;
    
    if (!findLayerInList(stackingContext->negZOrderList(), layer, position)) {
        if (!findLayerInList(stackingContext->normalFlowList(), layer, position)) {
            if (!findLayerInList(stackingContext->posZOrderList(), layer, position)) {
                ASSERT_NOT_REACHED();
                return notFound;
            }
        }
    }

    return position;
}

static bool compareLayerInSameStackingContext(RenderLayer* layerA, RenderLayer* layerB)
{
    RenderLayer* stackingContext = layerA->stackingContext();
    ASSERT(stackingContext == layerB->stackingContext());
    stackingContext->updateLayerListsIfNeeded();
    // We just need to iterate on the lists and see which one is first.
    size_t positionA = layerPositionInStackingContext(stackingContext, layerA);
    size_t positionB = layerPositionInStackingContext(stackingContext, layerB);
    return positionA < positionB;
}

static void fillWithLayerStackContextChain(RenderLayer* layer, Vector<RenderLayer*>& chain)
{
    for (; layer; layer = layer->stackingContext()) {
        chain.prepend(layer);
        if (layer->renderer()->isRenderView() || layer->renderer()->isRoot())
            return;
    }
}

static bool compareLayerOrder(RenderLayer* layerA, RenderLayer* layerB)
{
    ASSERT(layerA != layerB);
    
    if (layerA->stackingContext() == layerB->stackingContext())
        return compareLayerInSameStackingContext(layerA, layerB);
    
    Vector<RenderLayer*> stackContextChainA;
    fillWithLayerStackContextChain(layerA, stackContextChainA);
    
    Vector<RenderLayer*> stackContextChainB;
    fillWithLayerStackContextChain(layerB, stackContextChainB);
    
    size_t i = 0;
    for (; i < stackContextChainA.size() && i < stackContextChainB.size(); ++i) {
        if (stackContextChainA.at(i) != stackContextChainB.at(i))
            break;
    }
    if (!i)
        return true;
    if (i == stackContextChainA.size())
        return true;
    if (i == stackContextChainB.size())
        return false;
    ASSERT(i < stackContextChainA.size() && i < stackContextChainB.size());
    return compareLayerInSameStackingContext(stackContextChainA.at(i), stackContextChainB.at(i));
}

static bool compareWrappingContexts(WrappingContext* contextA, WrappingContext* contextB)
{
    RenderLayer* layerA = contextA->layer();
    RenderLayer* layerB = contextB->layer();
    return compareLayerOrder(layerA, layerB);
}

void WrappingContext::sortChildContextsIfNeeded()
{
    if (!m_hasDirtyChildContextOrder)
        return;
    // 1. If the wrap-contexts are in the same stacking-context use z-index or fail to document order if not specified.
    // 2. If the wrap-contexts are not in the same stacking contexts, find the common ancestor stacking context and follow the 
    // z-index on that or fail to document order if z-index is not specified
    std::sort(m_children.begin(), m_children.end(), compareWrappingContexts);
    m_hasDirtyChildContextOrder = false;
}

void WrappingContext::setHasDirtyChildContextsOrder()
{
    m_hasDirtyChildContextOrder = true;
}

void WrappingContext::resetExclusionsForFirstLayoutPass()
{
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children.at(i)->resetExclusionsForFirstLayoutPass();
    if (layer()->renderer()->isRenderBlock())
        toRenderBlock(layer()->renderer())->setNeedsLayoutForWrappingContextChange();
    if (layer()->renderer()->style()->isCSSExclusion())
        layer()->renderer()->setNeedsLayout(true);
}
    
void WrappingContext::markDescendantsForSecondLayoutPass()
{
    m_savedTransformationMatrix = layer()->renderer()->absoluteTransformationMatrix();
    m_savedExclusionBoundingBox = layer()->renderer()->absoluteBoundingBoxRect();
    
    sortChildContextsIfNeeded();
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children.at(i)->markDescendantsForSecondLayoutPass();
    if (layer()->renderer()->isRenderBlock())
        toRenderBlock(layer()->renderer())->setNeedsLayoutForWrappingContextChange();
}
    
void WrappingContext::addSiblingExclusions(WrappingContext* child, Vector<WrappingContext*>& list)
{
    size_t childIndex = m_children.find(child);
    if (childIndex == notFound)
        return;
    
    ++childIndex;
    for (; childIndex < m_children.size(); ++childIndex) {
        WrappingContext* child = m_children.at(childIndex);
        if (child->layer()->renderer()->style()->isCSSExclusion())
            list.append(child);
    }
    
    if (parent() && layer()->renderer()->style()->wrapThrough() == WrapThroughWrap)
        parent()->addSiblingExclusions(this, list);
}

void WrappingContext::computeExclusionListForBlock(RenderBlock* block, Vector<WrappingContext*>& list)
{
    if (parent() && layer()->renderer()->style()->wrapThrough() == WrapThroughWrap)
        parent()->addSiblingExclusions(this, list);
    if (!m_children.size())
        return;
    size_t firstWrappingContext = 0;
    if (block->layer() && block->layer()->hasWrappingContext()) {
        // Just find the wrapping context and return all the following ones.
        firstWrappingContext = m_children.find(block->layer()->wrappingContext());
        if (firstWrappingContext == notFound)
            return;
    } else {
        RenderLayer* enclosingLayer = block->enclosingLayer();
        if (enclosingLayer != layer()) {
            // Find the first wrapping context layer that is after the enclosing layer.
            for (size_t i = 0; i < m_children.size(); ++i) {
                if (compareLayerOrder(enclosingLayer, m_children.at(i)->layer())) {
                    firstWrappingContext = i;
                    break;
                }
            }
        }
        // FIXME: we should only return the wrapping context with positive z-index relative to
        // the "layer()" stacking context.
    }
    for (size_t i = firstWrappingContext; i < m_children.size(); ++i) {
        WrappingContext* child = m_children.at(i);
        if (child->layer()->renderer()->style()->isCSSExclusion())
            list.append(child);
    }
}

static bool overlapping(int t0, int t1, int y0, int y1)
{
    return t0 <= y1 && t1 >= y0;
}

void WrappingContext::adjustLinePositionForExclusions(RenderBlock* block, RootInlineBox* lineBox, LayoutUnit& deltaOffset, const Vector<WrappingContext*>& exclusionsList)
{
    deltaOffset = 0;
    
    LayoutRect logicalVisualOverflow = lineBox->logicalVisualOverflowRect(lineBox->lineTop(), lineBox->lineBottom());
    LayoutUnit logicalOffset = std::min(lineBox->lineTopWithLeading(), logicalVisualOverflow.y());
    LayoutUnit lineHeight = std::max(lineBox->lineBottomWithLeading(), logicalVisualOverflow.maxY()) - logicalOffset;
    
    FloatPoint upperLeftCorner = block->localToAbsolute(FloatPoint(0, logicalOffset - block->logicalTop()), false, true);
    FloatPoint lowerRightCorner = block->localToAbsolute(FloatPoint(10, logicalOffset + lineHeight - block->logicalTop()), false, true);
    
    int t0 = (int)std::min(upperLeftCorner.y(), lowerRightCorner.y());
    int t1 = (int)std::max(upperLeftCorner.y(), lowerRightCorner.y());
    bool hadNoChange = false;
    
    // FIXME: This is totaly wrong because we don't account for transforms in deltaOffset.
    // It will be fixed when we will have the shapes in place.
    while (!hadNoChange) {
        hadNoChange = true;
        for (size_t i = 0; i < exclusionsList.size(); ++i) {
            WrappingContext* exclusion = exclusionsList.at(i);
            const IntRect& exclusionBoundingBox = exclusion->savedExclusionBoundingBox();
            if (overlapping(t0 + deltaOffset, t1 + deltaOffset, exclusionBoundingBox.y(), exclusionBoundingBox.maxY())) {
                int newDelta = std::max(deltaOffset + t0, exclusionBoundingBox.maxY()) - t0;
                if (newDelta == deltaOffset)
                    continue;
                deltaOffset = newDelta;
                // We need to start checking again.
                hadNoChange = false;
                break;
            }
        }
    }
}

} // namespace WebCore

