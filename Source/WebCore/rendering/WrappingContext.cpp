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
#include <wtf/PassOwnPtr.h>

namespace WebCore {

typedef WTF::HashMap<const RenderBlock*, WrappingContext*> WrappingContextMap;
static WrappingContextMap* s_wrappingContextMap = 0;

ExclusionAreaMaintainer* ExclusionAreaMaintainer::s_current = 0;

ExclusionAreaData::ExclusionAreaData(RenderBlock* block, WrappingContext* context)
    : m_block(block)
    , m_context(context)
{
}

ExclusionAreaData::~ExclusionAreaData()
{
}

void ExclusionAreaMaintainer::init(RenderBlock* block, WrappingContext* context)
{
    m_data = adoptPtr(new ExclusionAreaData(block, context));
    context->exclusionBoxesForBlock(block, m_data->boxes());
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

ExclusionBox* WrappingContext::addExclusionBox(const RenderBox* renderBox)
{
    // Make sure there was no exclusion box associated with the RenderBox before.
    ASSERT(m_boxes.find(renderBox) == m_boxes.end());
    RefPtr<ExclusionBox> box = ExclusionBox::create(const_cast<RenderBox*>(renderBox));
    m_boxes.set(renderBox, box);
    return box.get();
}

void WrappingContext::removeExclusionBox(const RenderBox* renderBox)
{
    ASSERT(m_boxes.find(renderBox) != m_boxes.end());
    m_boxes.remove(renderBox);
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

void WrappingContext::prepareExlusionRects()
{
    LayoutStateDisabler disableLayoutState(m_block->view());

    for (ExclusionBoxMap::iterator iter = m_boxes.begin(), end = m_boxes.end(); iter != end; ++iter) {
        ExclusionBox* box = iter->second.get();
        RenderBox* renderer = box->renderer();
        // FIXME: We should really just use transforms here.
        box->setBoundingBox(renderer->absoluteBoundingBoxRect());
        printf("  prepared exclusion (%d %d)\n", box->boundingBox().y(), box->boundingBox().maxY());
    }
}

void WrappingContext::pushParentExclusionBoxes(const RenderObject* parentWithNewWrappingContext, ExclusionBoxes& boxes)
{
    for (ExclusionBoxMap::iterator iter = m_boxes.begin(), end = m_boxes.end(); iter != end; ++iter) {
        ExclusionBox* box = iter->second.get();
        if (parentWithNewWrappingContext && !box->renderer()->isDescendantOf(parentWithNewWrappingContext))
            continue;
        boxes.append(box);
    }
    RenderBlock* parentContainingBlock = m_block->containingBlock();
    if (!parentContainingBlock)
        return;
    WrappingContext* context = contextForBlock(parentContainingBlock);
    if (!context)
        return;
    context->pushParentExclusionBoxes(parentWithNewWrappingContext, boxes);
}

void WrappingContext::exclusionBoxesForBlock(const RenderBlock* block, ExclusionBoxes& boxes)
{
    WrappingContext* context = contextForBlock(block);
    if (!context)
        return;
    const RenderObject* parentWithNewWrappingContext = context->parentWithNewWrappingContext(block);
    context->pushParentExclusionBoxes(parentWithNewWrappingContext, boxes);
}

} // namespace WebCore

