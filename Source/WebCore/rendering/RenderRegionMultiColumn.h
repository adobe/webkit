/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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

#ifndef RenderRegionMultiColumn_h
#define RenderRegionMultiColumn_h

#include "RenderBlock.h"

namespace WebCore {

class RenderFlowThread;

class RenderRegionMultiColumn: public RenderBlock {
public:
    explicit RenderRegionMultiColumn(Node*, RenderFlowThread*);
    virtual void layoutBlock(bool relayoutChildren, int, BlockLayoutPass);
    
    virtual const char* renderName() const;
    
    virtual bool canHaveChildren() const { return false; }
    
    unsigned columnCount() const { return m_columnCount; }
    
    bool hasAutoHeight() const { return style()->logicalHeight().isAuto(); }
    bool hasAutoWidth() const { return style()->logicalWidth().isAuto(); }
    bool hasAutoHeightStyle() const  { return isHorizontalWritingMode() ? hasAutoHeight() : hasAutoWidth(); }
    
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    
    bool usesAutoHeight() const { return m_usesAutoHeight; }

    bool hasComputedAutoHeight() const { return document()->cssRegionsAutoHeightEnabled() && m_hasComputedAutoHeight; }
    LayoutUnit computedAutoHeight() const { return m_computedAutoHeight; }
    void setComputedAutoHeight(LayoutUnit computedAutoHeight);
    
    void resetComputedAutoHeight() {
        ASSERT(document()->cssRegionsAutoHeightEnabled());
        m_computedAutoHeight = 0;
        m_hasComputedAutoHeight = false;
    }
    
    virtual void computeLogicalHeight();
    
    virtual LayoutUnit minPreferredLogicalWidth() const;
    virtual LayoutUnit maxPreferredLogicalWidth() const;
    
    virtual bool isRenderRegionMultiColumn() const { return true; }
private:
    void updateColumnCount(unsigned newColumnCount);
    
    RenderFlowThread* m_flowThread;
    unsigned m_columnCount;

    // Store the computed region autoheight
    LayoutUnit m_computedAutoHeight;
    bool m_hasComputedAutoHeight;
    bool m_usesAutoHeight;
};

}

#endif // RenderRegionMultiColumn_h
