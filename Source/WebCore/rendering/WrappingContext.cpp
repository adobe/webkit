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

WrappingContext::WrappingContext(RenderBlock* block)
    : m_block(block)
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

