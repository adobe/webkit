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

#include "RenderView.h"
#include "RootInlineBox.h"
#include "ExclusionBox.h"
#include "RenderBlock.h"
#include "RenderLayer.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/StdLibExtras.h>

#include "Node.h"
#include "Element.h"

namespace WebCore {

typedef WTF::HashMap<const RenderBlock*, WrappingContext*> WrappingContextMap;
static WrappingContextMap* s_wrappingContextMap = 0;

typedef WTF::HashMap<const RenderView*, ExclusionBoxMap*> ExclusionsMapHolder;
static ExclusionsMapHolder* s_exclusions = 0;

ExclusionAreaMaintainer* ExclusionAreaMaintainer::s_current = 0;

static String nodeID(Node* node)
{
    return (node->isElementNode() && node->hasID()) ? static_cast<Element*>(node)->idForStyleResolution() : "";
}

ExclusionAreaData::ExclusionAreaData(RenderBlock* block, WrappingContext* context)
    : m_block(block)
    , m_context(context)
{
}

ExclusionAreaData::~ExclusionAreaData()
{
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
            if (!findLayerInList(stackingContext->posZOrderList(), layer, position))
                return notFound;
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
    if (positionA == notFound)
        return true;
    size_t positionB = layerPositionInStackingContext(stackingContext, layerB);
    if (positionB == notFound)
        return false;
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

static bool compareExclusions(const RefPtr<ExclusionBox>& boxA, const RefPtr<ExclusionBox>& boxB)
{
    RenderBox* renderBoxA = boxA->renderer();
    RenderBox* renderBoxB = boxB->renderer();

    if (renderBoxA->layer() && renderBoxB->layer())
        return compareLayerOrder(renderBoxA->layer(), renderBoxB->layer());
    
    // Use DOM order.
    unsigned short position = renderBoxB->node()->compareDocumentPosition(renderBoxA->node());
    return position & Node::DOCUMENT_POSITION_FOLLOWING;
}

void ExclusionAreaMaintainer::init(RenderBlock* block, WrappingContext* context)
{
    Node* nodeToPrint = block->node() ? block->node() : block->parent()->node();
    String name = (nodeToPrint->isElementNode() && nodeToPrint->hasID()) ? static_cast<Element*>(nodeToPrint)->idForStyleResolution() : "";
    printf("Layout and runs for \"%s\"\n", name.latin1().data());

    m_data = adoptPtr(new ExclusionAreaData(block, context));
    context->exclusionBoxesForBlock(block, m_data->boxes());
    if (!m_data->boxes().size()) {
        m_data.clear();
        return;
    }
    std::sort(m_data->boxes().begin(), m_data->boxes().end(), compareExclusions);

    size_t lastParentExclusion = notFound;
    for (RenderBlock* object = block; object && (object != context->block()); object = object->containingBlock()) {
        ASSERT(object->isDescendantOf(context->block()));
        if (object->isExclusionBox() && object->isBox() && object->containingBlock()) {
            ExclusionBox* box = WrappingContext::exclusionForBox(object);
            if (!box) {
                printf("no exclusion box for %p\n", object);
                ASSERT_NOT_REACHED();
                continue;
            }
            size_t i = m_data->boxes().find(box);
            if (i == notFound)
                continue;
            if (lastParentExclusion == notFound || i > lastParentExclusion)
                lastParentExclusion = i;
        }
    }

    if (lastParentExclusion != notFound) {
        ExclusionBoxes result;
        for (size_t i = lastParentExclusion + 1; i < m_data->boxes().size(); ++i)
            result.append(m_data->boxes().at(i).get());
        m_data->boxes().swap(result);
    }

    prepareExlusionRects();
}

void ExclusionAreaMaintainer::prepareExlusionRects()
{
    LayoutStateDisabler disableLayoutState(m_data->block()->view());

    for (size_t i = 0; i < m_data->boxes().size(); ++i) {
        ExclusionBox* box = m_data->boxes().at(i).get();
        RenderBox* renderer = box->renderer();
        // FIXME: We should really just use transforms here.
        box->setBoundingBox(renderer->absoluteBoundingBoxRect());
        printf("  prepared exclusion (%d %d)\n", box->boundingBox().y(), box->boundingBox().maxY());
    }
}

static bool overlapping(int t0, int t1, int y0, int y1)
{
    return t0 <= y1 && t1 >= y0;
}

void ExclusionAreaMaintainer::adjustLinePositionForExclusions(RootInlineBox* lineBox, LayoutUnit& deltaOffset)
{
    ASSERT(m_data.get());
    RenderBlock* block = m_data->block();
    LayoutStateDisabler disableLayoutState(block->view());

    deltaOffset = 0;
    
    LayoutRect logicalVisualOverflow = lineBox->logicalVisualOverflowRect(lineBox->lineTop(), lineBox->lineBottom());
    LayoutUnit logicalOffset = std::min(lineBox->lineTopWithLeading(), logicalVisualOverflow.y());
    LayoutUnit lineHeight = std::max(lineBox->lineBottomWithLeading(), logicalVisualOverflow.maxY()) - logicalOffset;
    
    FloatPoint upperLeftCorner = block->localToAbsolute(FloatPoint(0, logicalOffset), false, true);
    FloatPoint lowerRightCorner = block->localToAbsolute(FloatPoint(10, logicalOffset + lineHeight), false, true);
    
    int t0 = (int)std::min(upperLeftCorner.y(), lowerRightCorner.y());
    int t1 = (int)std::max(upperLeftCorner.y(), lowerRightCorner.y());
    
    printf("line (%d %d)\n", t0, t1);

    bool hadNoChange = false;
    
    // FIXME: This is totaly wrong because we don't account for transforms in deltaOffset.
    // It will be fixed when we will have the shapes in place.
    while (!hadNoChange) {
        hadNoChange = true;
        for (size_t i = 0; i < m_data->boxes().size(); ++i) {
            ExclusionBox* exclusion = m_data->boxes().at(i).get();
            const IntRect& exclusionBoundingBox = exclusion->boundingBox();
            printf("   exclusion (%d %d)\n", exclusionBoundingBox.y(), exclusionBoundingBox.maxY());
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

WrappingContext::WrappingContext(RenderBlock* block)
    : m_block(block)
    , m_state(ExclusionsLayoutPhase)
{
}

WrappingContext::~WrappingContext()
{
}

ExclusionBoxMap* WrappingContext::boxMapForView(RenderView* view, bool create)
{
    if (!s_exclusions) {
        if (!create)
            return 0;
        s_exclusions = new ExclusionsMapHolder();
    }

    ExclusionsMapHolder::iterator iter = s_exclusions->find(view);
    if (iter != s_exclusions->end())
        return iter->second;

    ExclusionBoxMap* map = new ExclusionBoxMap;
    s_exclusions->set(view, map);
    return map;
}

void WrappingContext::removeBoxMapForView(RenderView* view)
{
    if (!s_exclusions)
        return;
    ExclusionsMapHolder::iterator iter = s_exclusions->find(view);
    if (iter == s_exclusions->end())
        return;
    delete iter->second;
    s_exclusions->remove(iter);
    if (!s_exclusions->size()) {
        delete s_exclusions;
        s_exclusions = 0;
    }
}

ExclusionBox* WrappingContext::addExclusionBox(const RenderBox* renderBox)
{
    ExclusionBoxMap* boxes = boxMapForView(renderBox->view(), true);
    ExclusionBoxMap::iterator iter = boxes->find(renderBox);
    if (iter != boxes->end())
        return iter->second.get(); 
    
    RefPtr<ExclusionBox> box = ExclusionBox::create(const_cast<RenderBox*>(renderBox));
    boxes->set(renderBox, box);

    // Make sure the containing block knows we need a wrapping context.
    RenderBlock* containingBlock = renderBox->containingBlock();
    if (containingBlock)
        createContextForBlockIfNeeded(containingBlock);

    printf("added new exclusion box %p - \"%s\"\n", renderBox, nodeID(renderBox->node()).latin1().data());
    for (RenderObject* o = renderBox->parent(); o ; o = o->parent())
        printf("-- parent box %p - \"%s\"\n", o, nodeID(o->node()).latin1().data());

    return box.get();
}

void WrappingContext::removeExclusionBox(const RenderBox* renderBox)
{
    printf("removed exclusion box %p\n", renderBox);

    ExclusionBoxMap* boxes = boxMapForView(renderBox->view());
    if (!boxes)
        return;

    ASSERT(boxes->find(renderBox) != boxes->end());
    boxes->remove(renderBox);
    
    if (!boxes->size())
        removeBoxMapForView(renderBox->view());
}

ExclusionBox* WrappingContext::exclusionForBox(const RenderBox* renderBox)
{
    ExclusionBoxMap* boxes = boxMapForView(renderBox->view());
    if (!boxes)
        return 0;

    ExclusionBoxMap::const_iterator iter = boxes->find(renderBox);
    return (iter == boxes->end()) ? 0 : iter->second.get();
}

WrappingContext* WrappingContext::lookupContextForBlock(const RenderBlock* block)
{
    if (!s_wrappingContextMap)
        return 0;
    WrappingContextMap::iterator iter = s_wrappingContextMap->find(block);
    if (iter == s_wrappingContextMap->end())
        return 0;
    return iter->second;
}

bool WrappingContext::blockHasOwnWrappingContext(const RenderBlock* block)
{
    return lookupContextForBlock(block);
}

WrappingContext* WrappingContext::createContextForBlockIfNeeded(const RenderBlock* block)
{
    WrappingContext* context = lookupContextForBlock(block);
    if (!context) {
        context = new WrappingContext(const_cast<RenderBlock*>(block));
        s_wrappingContextMap = new WrappingContextMap();
        s_wrappingContextMap->set(block, context);
    }
    return context;
}

void WrappingContext::removeContextForBlock(const RenderBlock* block)
{
    if (!s_wrappingContextMap)
        return;
    WrappingContextMap::iterator iter = s_wrappingContextMap->find(block);
    if (iter == s_wrappingContextMap->end())
        return;
    delete iter->second;
    s_wrappingContextMap->remove(iter);
    if (!s_wrappingContextMap->size()) {
        delete s_wrappingContextMap;
        s_wrappingContextMap = 0;
    }
}

WrappingContext* WrappingContext::contextForBlock(const RenderBlock* block)
{
    WrappingContext* context = lookupContextForBlock(block);
    while (!context && (block = block->containingBlock()))
        context = lookupContextForBlock(block);
    return context;
}

const RenderObject* WrappingContext::parentWithNewWrappingContext(const RenderBlock* block)
{
    for (const RenderObject* object = block; object && object != m_block; object = object->parent()) {
        if (object->style()->wrapThrough() == WrapThroughNone)
            return object;
    }
    return 0;
}

static bool compareRenderBoxExclusions(const RenderBox* renderBoxA, const RenderBox* renderBoxB)
{
    // Return the exclusions in reverse z-index order.
    if (renderBoxA->layer() && renderBoxB->layer())
        return compareLayerOrder(renderBoxB->layer(), renderBoxA->layer());
    // Use the reverse DOM order.
    unsigned short position = renderBoxA->node()->compareDocumentPosition(renderBoxB->node());
    return position & Node::DOCUMENT_POSITION_FOLLOWING;
}

void WrappingContext::sortPositionedObjects(Vector<RenderBox*>& list)
{
    printf("sorting exclusions\n");
    std::sort(list.begin(), list.end(), compareRenderBoxExclusions);
}

void WrappingContext::pushParentExclusionBoxes(const RenderObject* parentWithNewWrappingContext, ExclusionBoxes& resultBoxes)
{
    printf("push parents %p for \"%s\"\n", m_block, nodeID(m_block->node()).latin1().data());

    if (RenderBlock* parentContainingBlock = m_block->containingBlock())
        if (WrappingContext* context = contextForBlock(parentContainingBlock))
            context->pushParentExclusionBoxes(parentWithNewWrappingContext, resultBoxes);

    ExclusionBoxMap* boxes = boxMapForView(m_block->view());
    if (!boxes)
        return;

    for (ExclusionBoxMap::iterator iter = boxes->begin(), end = boxes->end(); iter != end; ++iter) {
        ExclusionBox* box = iter->second.get();
        if (box->renderer()->containingBlock() != m_block)
            continue;
        if (parentWithNewWrappingContext && !box->renderer()->isDescendantOf(parentWithNewWrappingContext))
            continue;
        resultBoxes.append(box);

        printf("-- exclusions check %p - \"%s\"\n", box->renderer(), nodeID(box->renderer()->node()).latin1().data());
        RenderBlock* cb = box->renderer()->containingBlock();
        if (cb)
            printf("-- containing block check %p - \"%s\"\n", cb, nodeID(cb->node()).latin1().data());
    }
}

void WrappingContext::exclusionBoxesForBlock(const RenderBlock* block, ExclusionBoxes& boxes)
{
    const RenderObject* parentWithNewWrappingContext = this->parentWithNewWrappingContext(block);
    pushParentExclusionBoxes(parentWithNewWrappingContext, boxes);
}

} // namespace WebCore

