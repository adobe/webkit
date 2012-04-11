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

#ifndef WrappingContext_h
#define WrappingContext_h

#include "TransformationMatrix.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class RenderBlock;
class RootInlineBox;
class RenderLayer;
class WrappingContext;

typedef Vector<WrappingContext*> WrappingContextList;

class WrappingContext {
    WTF_MAKE_NONCOPYABLE(WrappingContext);
public:
    static PassOwnPtr<WrappingContext> create(RenderLayer* layer);
    ~WrappingContext();
    
    void didAddOnlyThisContext();
    void willRemoveOnlyThisContext();
    
    void didAddLayer();
    void willRemoveLayer();
    
    void setHasDirtyChildContextsOrder();
    void sortChildContextsIfNeeded();
    WrappingContext* childAt(size_t i) const { return m_children.at(i); }
    size_t childCount() const { return m_children.size(); }
    
    WrappingContext* parent() const { return m_parent; }
    RenderLayer* layer() const { return m_layer; }
    
    void resetExclusionsForFirstLayoutPass();
    void markDescendantsForSecondLayoutPass();
    void addSiblingExclusions(WrappingContext*, Vector<WrappingContext*>&);
    void computeExclusionListForBlock(RenderBlock*, Vector<WrappingContext*>&);
    void adjustLinePositionForExclusions(RenderBlock*, RootInlineBox*, LayoutUnit& deltaOffset, const Vector<WrappingContext*>& exclusionsList);
    
    const TransformationMatrix& savedTransformationMatrix() const { return m_savedTransformationMatrix; }
    const IntRect& savedExclusionBoundingBox() const { return m_savedExclusionBoundingBox; }
private:
    WrappingContext(RenderLayer*);
    
    void addChildContext(WrappingContext*);
    void removeChildContext(WrappingContext*);
    void setParent(WrappingContext* parent) { m_parent = parent; }

    RenderLayer* m_layer;
    
    WrappingContext* m_parent;
    WrappingContextList m_children;
    
    TransformationMatrix m_savedTransformationMatrix;
    IntRect m_savedExclusionBoundingBox;
    
    bool m_hasDirtyChildContextOrder;
};

} // Namespace WebCore

#endif // WrappingContext_h
