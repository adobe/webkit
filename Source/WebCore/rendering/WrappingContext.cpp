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

#include "RenderLayer.h"
#include "RenderObject.h"
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
        chain.append(layer);
        if (layer->renderer()->isRenderView() || !layer->renderer()->isRoot())
            return;
    }
}

static bool compareWrappingContexts(const WrappingContext* contextA, const WrappingContext* contextB)
{
    RenderLayer* layerA = contextA->layer();
    RenderLayer* layerB = contextB->layer();
    ASSERT(layerA != layerB);
    
    if (layerA->stackingContext() == layerB->stackingContext())
        return compareLayerInSameStackingContext(contextA->layer(), contextB->layer());
    
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
    ASSERT(i < stackContextChainA.size() && i < stackContextChainB.size());
    return compareLayerInSameStackingContext(stackContextChainA.at(i), stackContextChainB.at(i));
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

} // namespace WebCore

