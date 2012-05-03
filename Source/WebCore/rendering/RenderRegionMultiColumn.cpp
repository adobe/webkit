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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
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
#include "RenderRegionMultiColumn.h"

#include "LayoutRepainter.h"
#include "LayoutTypes.h"
#include "RenderFlowThread.h"
#include "RenderRegion.h"
#include "RenderStyle.h"
#include "RenderView.h"

#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

RenderRegionMultiColumn::RenderRegionMultiColumn(Node* node, RenderFlowThread* flowThread)
    : RenderBlock(node)
    , m_flowThread(flowThread)
    , m_columnCount(0)
    , m_computedAutoHeight(0)
    , m_hasComputedAutoHeight(false)
    , m_usesAutoHeight(false)
{
}

void RenderRegionMultiColumn::updateColumnCount(unsigned newColumnCount)
{
    if (newColumnCount == m_columnCount)
        return;
    
    unsigned columns = m_columnCount;
    m_columnCount = newColumnCount;
    
    for (; columns < newColumnCount; ++columns) {
        // Add new columns.
        RenderRegion* region = new (renderArena()) RenderRegion(document(), m_flowThread);
    
        RefPtr<RenderStyle> newStyle(RenderStyle::createAnonymousStyle(style()));
        newStyle->setDisplay(BLOCK);
        newStyle->setRegionOverflow(BreakRegionOverflow);
        newStyle->setOverflowX(OHIDDEN);
        newStyle->setOverflowY(OHIDDEN);
        newStyle->setWidth(Length(0, Intrinsic));
        newStyle->setHeight(Length(0, Intrinsic));
        newStyle->font().update(0);
        region->setParentMultiColumnRegion(this);
        region->setStyle(newStyle.release());
        
        addChild(region);
    }
    
    for (; columns > newColumnCount; --columns) {
        // Delete the last column.
        RenderObject* lastColumn = lastChild();
        removeChild(lastColumn);
        lastColumn->destroy();
    }
}

void RenderRegionMultiColumn::layoutBlock(bool relayoutChildren, int, BlockLayoutPass)
{
    ASSERT(needsLayout());

    if (!relayoutChildren && simplifiedLayout())
        return;

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    if (inRenderFlowThread()) {
        // Regions changing widths can force us to relayout our children.
        if (logicalWidthChangedInRegions())
            relayoutChildren = true;
    }
    computeInitialRegionRangeForBlock();

    setLogicalHeight(0);
    // We need to call both of these because we grab both crossAxisExtent and mainAxisExtent in layoutFlexItems.
    computeLogicalWidth();
    computeLogicalHeight();

    m_overflow.clear();

    LayoutRect columnRect = contentBoxRect();
    
    // Calculate our column width and column count.
    unsigned desiredColumnCount = 1;
    LayoutUnit desiredColumnWidth = contentLogicalWidth();
    
    LayoutUnit availWidth = desiredColumnWidth;
    LayoutUnit colGap = columnGap();
    LayoutUnit colWidth = max<LayoutUnit>(1, LayoutUnit(style()->columnWidth()));
    int colCount = max<int>(1, style()->columnCount());

    if (style()->hasAutoColumnWidth() && !style()->hasAutoColumnCount()) {
        desiredColumnCount = colCount;
        desiredColumnWidth = max<LayoutUnit>(0, (availWidth - ((desiredColumnCount - 1) * colGap)) / desiredColumnCount);
    } else if (!style()->hasAutoColumnWidth() && style()->hasAutoColumnCount()) {
        desiredColumnCount = max<LayoutUnit>(1, (availWidth + colGap) / (colWidth + colGap));
        desiredColumnWidth = ((availWidth + colGap) / desiredColumnCount) - colGap;
    } else {
        desiredColumnCount = max<LayoutUnit>(min<LayoutUnit>(colCount, (availWidth + colGap) / (colWidth + colGap)), 1);
        desiredColumnWidth = ((availWidth + colGap) / desiredColumnCount) - colGap;
    }

    updateColumnCount(desiredColumnCount);
    colGap = desiredColumnCount > 1 ? (availWidth - desiredColumnCount * desiredColumnWidth) / (desiredColumnCount - 1) : 0;
    columnRect.setWidth(desiredColumnWidth);
    int remainder = availWidth - desiredColumnCount * desiredColumnWidth - (colGap * (desiredColumnWidth - 1));
    
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isRenderRegion())
            continue;
        RenderRegion* childRegion = toRenderRegion(child);
        childRegion->setLogicalLeft(columnRect.x());
        childRegion->setLogicalTop(columnRect.y());
        if (childRegion->updateIntrinsicSizeIfNeeded(columnRect.size()))
            childRegion->setChildNeedsLayout(true);
        childRegion->layoutIfNeeded();
        if (remainder > 0) {
            // Adjust the size, so that we don't end up having a large gap the end.
            remainder--;
            columnRect.move(1, 0);
        }
        columnRect.move(columnRect.width() + colGap, 0);
    }
    
    computeRegionRangeForBlock();

    statePusher.pop();

    updateLayerTransform();

    // Update our scroll information if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    updateScrollInfoAfterLayout();

    repainter.repaintAfterLayout();

    setNeedsLayout(false);
}

void RenderRegionMultiColumn::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    
    if (!oldStyle) {
        // make sure we have at least a column
        updateColumnCount(1);
    }
    
    bool didUseAutoHeight = m_usesAutoHeight;
    m_usesAutoHeight = hasAutoHeightStyle();
    if (m_flowThread && didUseAutoHeight != m_usesAutoHeight) {
        if (m_usesAutoHeight)
            view()->incrementAutoHeightRegions();
        else
            view()->decrementAutoHeightRegions();
        setNeedsLayout(true);
        m_flowThread->setNeedsLayout(true);
    }
}

void RenderRegionMultiColumn::computeLogicalHeight()
{
    RenderBlock::computeLogicalHeight();
    if (document()->cssRegionsAutoHeightEnabled() && !view()->inFirstLayoutPhaseOfRegionsAutoHeight()
        && usesAutoHeight() && hasComputedAutoHeight()) {
        LayoutUnit newLogicalHeight = computedAutoHeight() + borderAndPaddingLogicalHeight();
        if (newLogicalHeight > logicalHeight())
            setLogicalHeight(newLogicalHeight);
    }
}

LayoutUnit RenderRegionMultiColumn::minPreferredLogicalWidth() const
{
    return m_flowThread ? m_flowThread->minPreferredLogicalWidth() : RenderBlock::minPreferredLogicalWidth();
}

LayoutUnit RenderRegionMultiColumn::maxPreferredLogicalWidth() const
{
    return m_flowThread ? m_flowThread->maxPreferredLogicalWidth() : RenderBlock::maxPreferredLogicalWidth();
}

void RenderRegionMultiColumn::setComputedAutoHeight(LayoutUnit computedAutoHeight)
{
    ASSERT(document()->cssRegionsAutoHeightEnabled());
    m_computedAutoHeight = computedAutoHeight;
    m_hasComputedAutoHeight = true;
}
    
bool RenderRegionMultiColumn::hasAutoHeightStyle() const
{
    RenderStyle* styleToUse = style();
    bool hasAnchoredTopAndBottom = (isPositioned() || isRelPositioned()) && styleToUse->logicalTop().isSpecified() && styleToUse->logicalBottom().isSpecified();
    return !hasAnchoredTopAndBottom && styleToUse->logicalHeight().isAuto();
}


const char* RenderRegionMultiColumn::renderName() const
{
    return "RenderRegionMultiColumn";
}

} // namespace WebCore
