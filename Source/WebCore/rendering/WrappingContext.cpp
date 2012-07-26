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
#include "RenderFlowThread.h"
#include "RenderRegion.h"
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

ExclusionAreaData::ExclusionAreaData(RenderBlock* block)
    : m_block(block)
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
    unsigned short position = renderBoxA->node()->compareDocumentPosition(renderBoxB->node());
    return (position & Node::DOCUMENT_POSITION_FOLLOWING);
}

void ExclusionAreaMaintainer::init(RenderBlock* block)
{
    s_current = this;
    m_data = adoptPtr(new ExclusionAreaData(block));
    m_context->exclusionBoxesForBlock(block, m_data->boxes());
    if (!m_data->boxes().size()) {
        m_data.clear();
        return;
    }
    std::sort(m_data->boxes().begin(), m_data->boxes().end(), compareExclusions);

    size_t lastParentExclusion = notFound;
    for (RenderBlock* object = block; object; object = object->containingBlock()) {
        if (object->isExclusionBox()) {
            ExclusionBox* box = WrappingContext::exclusionForBox(object);
            if (!box) {
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

#ifndef NDEBUG
    // Check that we are not part of the current exclusion set.
    for (size_t i = 0; i < m_data->boxes().size(); ++i) {
        RenderObject* renderer = m_data->boxes().at(i)->renderer();
        ASSERT(renderer != block);
    }
#endif
}

void ExclusionAreaMaintainer::destroy()
{
    s_current = m_previous;
}

void ExclusionAreaMaintainer::prepareExlusionRects()
{
    LayoutStateDisabler disableLayoutState(m_data->block()->view());
    for (size_t i = 0; i < m_data->boxes().size(); ++i) {
        ExclusionBox* box = m_data->boxes().at(i).get();
        RenderBox* renderer = box->renderer();
        if (box->isPositioned() && !renderer->isDescendantOf(m_context->block()))
            continue;
        
        // FIXME: We should really just use transforms here.
        box->setBoundingBox(renderer->absoluteBoundingBoxRect());
        box->setIsPositioned(true);
    }
}

void ExclusionAreaMaintainer::layoutExclusionBoxes()
{
    LayoutStateDisabler disableLayoutState(m_data->block()->view());
    
    RenderBlock* block = m_data->block();
    for (size_t i = m_data->boxes().size(); i > 0; --i) {
        ExclusionBox* box = m_data->boxes().at(i - 1).get();
        RenderBox* renderer = box->renderer();
        if (!renderer->isDescendantOf(block)) {
            ASSERT(!renderer->needsLayout());
            continue;
        }
        renderer->setNeedsLayout(true, MarkOnlyThis);
        renderer->layoutIfNeeded();
    }
}

static bool overlapping(const LayoutUnit& t0, const LayoutUnit& t1, const LayoutUnit& y0, const LayoutUnit& y1)
{
    return t0 <= y1 && t1 >= y0;
}

static bool insertExclusion(ExclusionAreaMaintainer::LineSegments& segments, LayoutUnit left, LayoutUnit right)
{
    if (!segments.size())
        return false;

    // Find the first and the last segments to fit left and right.
    size_t first = notFound, last = notFound;
    
    for (size_t i = 0; i < segments.size(); ++i) {
        if (left <= segments.at(i).right && right >= segments.at(i).left) {
            first = i;
            break;
        }
    }

    if (first == notFound)
        return true;

    last = first;
    for (size_t i = first + 1; i < segments.size(); ++i) {
        if (segments.at(i).left >= right)
            break;
        last = i;
    }

    ASSERT(first <= last);
    if (first == last) {
        ExclusionAreaMaintainer::LineSegment& firstSegment = segments.at(first);

        if (left <= firstSegment.left && right >= firstSegment.right) {
            // Full coverage. Just delete the segment.
            segments.remove(first);
        } else if (left <= firstSegment.left) {
            // Left part covered only.
            firstSegment.left = right;
        } else if (right >= firstSegment.right) {
            // Right part covered only.
            firstSegment.right = left;
        } else {
            // Exclusion is inside segment. Split the one we had.
            ExclusionAreaMaintainer::LineSegment newSegment;
            newSegment.left = right;
            newSegment.right = firstSegment.right;
            segments.insert(first + 1, newSegment);
            // Update the old segment to end before the exclusion.
            firstSegment.right = left;
        }
    } else {
        size_t middle = last - first - 1;
        if (middle)
            segments.remove(first + 1, last - first - 1);
        last -= middle;

        ExclusionAreaMaintainer::LineSegment& firstSegment = segments.at(first);
        firstSegment.right = std::min(left, firstSegment.right);

        ExclusionAreaMaintainer::LineSegment& lastSegment = segments.at(last);
        firstSegment.left = std::min(right, firstSegment.left);

        if (!firstSegment.length()) {
            segments.remove(first);
            --last;
        }

        if (!lastSegment.length())
            segments.remove(last);
    }

    return segments.size();
}

bool ExclusionAreaMaintainer::getSegments(LayoutUnit logicalWidth, LayoutUnit logicalHeight, LayoutUnit lineHeight, LayoutUnit& deltaOffset, ExclusionAreaMaintainer::LineSegments& segments)
{
    ASSERT(m_data.get());
    RenderBlock* block = m_data->block();
    LayoutStateDisabler disableLayoutState(block->view());

    deltaOffset = 0;
    
    LayoutPoint topLeft(0, logicalHeight);
    if (block->hasColumns()) {
        LayoutSize columnOffset;
        block->adjustForColumns(columnOffset, topLeft);
        topLeft.move(columnOffset);
    }
    
    FloatQuad absoluteQuad = block->localToContainerQuad(FloatQuad(LayoutRect(topLeft, LayoutSize(logicalWidth, lineHeight))), 0);

    FloatRect boundingBox = absoluteQuad.boundingBox();
    
    LayoutUnit x0 = LayoutUnit(boundingBox.x());
    LayoutUnit x1 = LayoutUnit(boundingBox.maxX());

    LayoutUnit t0 = LayoutUnit(boundingBox.y());
    LayoutUnit t1 = LayoutUnit(boundingBox.maxY());
    
    bool hadAnyExclusions = false;
    bool hadNoOffsetChange = false;
    
    // FIXME: This is totaly wrong because we don't account for transforms in deltaOffset.
    // It will be fixed when we will have the shapes in place.
    while (!hadNoOffsetChange) {
        segments.clear();
        LineSegment infSegment;
        infSegment.left = ZERO_LAYOUT_UNIT;
        infSegment.right = x1 - x0;
        segments.append(infSegment);
        hadNoOffsetChange = true;
        hadAnyExclusions = false;
        for (size_t i = 0; i < m_data->boxes().size(); ++i) {
            ExclusionBox* exclusion = m_data->boxes().at(i).get();
            const LayoutRect& exclusionBoundingBox = exclusion->boundingBox();
            if (overlapping(t0 + deltaOffset, t1 + deltaOffset, exclusionBoundingBox.y(), exclusionBoundingBox.maxY())) {
                hadAnyExclusions = true;
                switch (exclusion->renderer()->style()->wrapFlow()) {
                    // FIXME: implement WrapFlowMaximum.
                    case WrapFlowMaximum:
                    case WrapFlowBoth:
                        if (insertExclusion(segments, exclusionBoundingBox.x() - x0, exclusionBoundingBox.maxX() - x0))
                            continue;
                        break;
                    case WrapFlowStart:
                        if (insertExclusion(segments, exclusionBoundingBox.x() - x0, MAX_LAYOUT_UNIT / 2))
                            continue;
                        break;
                    case WrapFlowEnd:
                        if (insertExclusion(segments, MIN_LAYOUT_UNIT / 2, exclusionBoundingBox.maxX() - x0))
                            continue;
                        break;
                    case WrapFlowClear:
                        // We cannot use this line at all.
                        break;
                    case WrapFlowAuto:
                        ASSERT_NOT_REACHED();
                }
                LayoutUnit newDelta = std::max(deltaOffset + t0, exclusionBoundingBox.maxY()) - t0;
                if (newDelta == deltaOffset)
                    continue;
                deltaOffset = newDelta;
                // We need to start checking again.
                hadNoOffsetChange = false;
                break;
            }
        }
    }

    return hadAnyExclusions;
}

WrappingContext::WrappingContext(RenderBlock* block)
    : m_block(block)
    , m_isDisabled(false)
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

    return box.get();
}

void WrappingContext::removeExclusionBox(const RenderBox* renderBox)
{
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
        if (!s_wrappingContextMap)
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
    while (!context && (block = block->containingBlock())) {
        context = lookupContextForBlock(block);
        if (!context && block->isRenderFlowThread()) {
            const RenderFlowThread* flowThread = toRenderFlowThread(block);
            if (RenderRegion* region = flowThread->firstRegion()) {
                if (RenderBlock* containerBlock = region->containingBlock()) {
                    block = containerBlock;
                    context = lookupContextForBlock(block);
                }
            }
        }
    }
    return context;
}

const RenderObject* WrappingContext::parentWithNewWrappingContext(const RenderBlock* block)
{
    for (const RenderObject* object = block; object && object != m_block; object = object->parent()) {
        if (object->style()->wrapThrough() == WrapThroughNone || object->style()->isFloating())
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

void WrappingContext::pushParentExclusionBoxes(const RenderObject* parentWithNewWrappingContext, ExclusionBoxes& resultBoxes)
{
    if (m_block->isRenderFlowThread()) {
        RenderFlowThread* flowThread = toRenderFlowThread(m_block);
        if (RenderRegion* region = flowThread->firstRegion()) {
            if (RenderBlock* parentContainingBlock = region->containingBlock())
                exclusionBoxesForBlock(parentContainingBlock, resultBoxes);
        }
        return;
    }
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
    }
}

void WrappingContext::exclusionBoxesForBlock(const RenderBlock* block, ExclusionBoxes& boxes)
{
    const RenderObject* parentWithNewWrappingContext = this->parentWithNewWrappingContext(block);
    pushParentExclusionBoxes(parentWithNewWrappingContext, boxes);
}

} // namespace WebCore

