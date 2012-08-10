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

#include "IntRect.h"
#include "LayoutTypes.h"
#include <wtf/Noncopyable.h>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class ExclusionBox;
class RenderBox;
class RenderBlock;
class RenderObject;
class RenderView;
class WrappingContext;
class RootInlineBox;
class XShape;

typedef HashMap<const RenderBox*, RefPtr<ExclusionBox> > ExclusionBoxMap;
typedef Vector<RefPtr<ExclusionBox> > ExclusionBoxes;
typedef HashMap<const ExclusionBox*, RefPtr<XShape> > ShapesMap;
class ExclusionAreaData {
public:
    ExclusionAreaData(RenderBlock*);
    ~ExclusionAreaData();

    const ExclusionBoxes& boxes() const { return m_boxes; }
    ExclusionBoxes& boxes() { return m_boxes; }

    RenderBlock* block() const { return m_block; }

    void setBoundingBox(const IntRect& boundingBox) { m_boundingBox = boundingBox; }
    const IntRect& boundingBox() const { return m_boundingBox; }

    XShape* shapeInside() const { return m_shapeInside.get(); }
    void setShapeInside(PassRefPtr<XShape>);

    ShapesMap& shapesMap() { return m_shapesMap; }

private:
    ExclusionBoxes m_boxes;
    RefPtr<XShape> m_shapeInside;
    ShapesMap m_shapesMap;
    RenderBlock* m_block;
    IntRect m_boundingBox;
};

class ExclusionAreaMaintainer {
public:
    ExclusionAreaMaintainer(RenderBlock* block, WrappingContext* context)
        : m_previous(s_current)
        , m_context(context)
    {
        if (m_context)
            init(block);
    }
    ~ExclusionAreaMaintainer()
    {
        if (m_context)
            destroy(); 
    }

    bool hasExclusionBoxes() const { return m_data.get() && m_data->boxes().size(); }
    ExclusionAreaData* data() const { return m_data.get(); }

    void prepareExlusionRects();
    void layoutExclusionBoxes();

    static ExclusionAreaMaintainer* active() { return s_current; }
    static bool shouldDisableExclusions();

    struct LineSegment {
        LayoutUnit left;
        LayoutUnit right;
        LayoutUnit length() const { return std::max(right - left, ZERO_LAYOUT_UNIT); }
    };
    typedef Vector<LineSegment> LineSegments;
    bool getSegments(LayoutUnit logicalWidth, LayoutUnit logicalHeight, LayoutUnit lineHeight, LayoutUnit& deltaOffset, LineSegments&);
private:
    void init(RenderBlock*);
    void destroy();
    
    static ExclusionAreaMaintainer* s_current;

    ExclusionAreaMaintainer* m_previous;
    OwnPtr<ExclusionAreaData> m_data;
    WrappingContext* m_context;
};

class WrappingContext {
    WTF_MAKE_NONCOPYABLE(WrappingContext);
public:

    static bool blockHasOwnWrappingContext(const RenderBlock*);
    static WrappingContext* contextForBlock(const RenderBlock*);

    static WrappingContext* createContextForBlockIfNeeded(const RenderBlock*);
    static void removeContextForBlock(const RenderBlock*);

    static ExclusionBox* addExclusionBox(const RenderBox*);
    static void removeExclusionBox(const RenderBox*);
    static ExclusionBox* exclusionForBox(const RenderBox* renderBox);

    RenderBlock* block() const { return m_block; }

    void exclusionBoxesForBlock(const RenderBlock*, ExclusionBoxes&);

    static ExclusionBoxMap* boxMapForView(RenderView*, bool create = false);
    static void removeBoxMapForView(RenderView* view);

    void setIsDisabled(bool value) { m_isDisabled = value; }
    bool isDisabled() const { return m_isDisabled; }
private:
    WrappingContext(RenderBlock*);
    ~WrappingContext();

    static WrappingContext* lookupContextForBlock(const RenderBlock*);

    const RenderObject* parentWithNewWrappingContext(const RenderBlock*);
    void pushParentExclusionBoxes(const RenderObject* parentWithNewWrappingContext, ExclusionBoxes&);

    RenderBlock* m_block;
    bool m_isDisabled;
};

inline bool ExclusionAreaMaintainer::shouldDisableExclusions() 
{ 
    return !s_current || (s_current->m_context && s_current->m_context->isDisabled()); 
}

} // Namespace WebCore

#endif // WrappingContext_h
