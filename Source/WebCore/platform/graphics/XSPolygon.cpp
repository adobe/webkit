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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XSPolygon.h"
#include <wtf/MathExtras.h>

namespace WebCore {

struct MaxEdgeYComparator {    
    bool operator() (const XSEdge* e1, const XSEdge* e2) const 
    {       
        return e1->maxY() < e2->maxY();
    }
    
    bool operator() (const XSEdge* e, float y) const 
    {       
        return e->maxY() < y;
    }
};

struct MinEdgeYComparator {    
    bool operator() (const XSEdge* e1, const XSEdge* e2) const 
    {       
        return e1->minY() < e2->minY();
    }
    
    bool operator() (const XSEdge* e, float y) const
    {       
        return e->minY() < y;
    }
};

struct IntersectionXComparator {
    bool operator() (const XSIntersection& i1, const XSIntersection& i2) const 
    {       
        return (i1.x == i2.x) ? i1.type < i2.type : i1.x < i2.x;
    }
};

XSEdgeTree::XSEdgeTree(const Vector<XSEdge*>& sortedEdges) 
    : center(0)
    , aboveCenter(0)
    , belowCenter(0)
{
    if (sortedEdges.size() > 0)
        initEdgeTree(this, sortedEdges);
}

void XSEdgeTree::initEdgeTree(XSEdgeTree* node, const Vector<XSEdge*>& edges)
{
    size_t edgesSize = edges.size();  
    size_t centerIndex = edgesSize / 2;
    node->center = (edges[centerIndex]->minY() + edges[centerIndex]->maxY()) / 2;
    
    Vector<XSEdge*> aboveCenterEdges;
    Vector<XSEdge*> belowCenterEdges;
    
    for(size_t i = 0; i < edgesSize; i++) {
        XSEdge *edge = edges[i];
        if (edge->maxY() < node->center)
            aboveCenterEdges.append(edge);
        else if (edge->minY() > node->center)
            belowCenterEdges.append(edge);
        else
            node->overlapCenter.append(edge);
    }
    
    // FIXME: create two sorted copies of overlapCenter: overlapCenterMinY, overlapCenterMaxY, use them in find...
    
    if (aboveCenterEdges.size() > 0)
        node->aboveCenter = new XSEdgeTree(aboveCenterEdges);
    if (belowCenterEdges.size() > 0)
        node->belowCenter = new XSEdgeTree(belowCenterEdges);
}

void XSEdgeTree::findEdgesThatOverlapY(float y, Vector<XSEdge*>& rv) const
{
    if (y < center) {
        if (aboveCenter) aboveCenter->findEdgesThatOverlapY(y, rv);
    }
    else if (y > center) {
        if (belowCenter) belowCenter->findEdgesThatOverlapY(y, rv);
    }
    
    size_t overlapEdgesSize = overlapCenter.size();
    for (size_t i = 0; i < overlapEdgesSize; i++) {
        XSEdge *edge = overlapCenter[i];
        if (y >= edge->minY() && y <= edge->maxY())
            rv.append(edge);
    }
}

void XSEdgeTree::findEdgesThatOverlapInterval(float yMin, float yMax, Vector<XSEdge*>& rv) const
{
    if (yMin < center) {
        if (aboveCenter) aboveCenter->findEdgesThatOverlapInterval(yMin, yMax, rv);
    }
    if (yMax > center) {
        if (belowCenter) belowCenter->findEdgesThatOverlapInterval(yMin, yMax, rv);
    }
    
    size_t overlapEdgesSize = overlapCenter.size();
    for (size_t i = 0; i < overlapEdgesSize; i++) {
        XSEdge *edge = overlapCenter[i];
        if (yMax > edge->minY() && yMin < edge->maxY())
            rv.append(edge);
    }
}

XSPolygon::XSPolygon(PassOwnPtr<Vector<float> > coordinates, WindRule fillRule)
    : XShape()
    , m_coordinates(coordinates)
    , m_fillRule(fillRule) 
{
    // FIXME: special cases for (length ==) 0, 1, 2  points
    // FIXME: assuming open polygons for now
    
    size_t nVertices = numberOfVertices();
    m_edges.resize(nVertices);
    Vector<XSEdge*> sortedEdgesMinY(nVertices);
    
    float minX = getXAt(0);
    float minY = getYAt(0);
    float maxX = minX;
    float maxY = minY;

    for (size_t i = 0; i < nVertices; i++) {
        float x = getXAt(i);
        float y = getYAt(i);
        
        minX = std::min(x, minX);
        maxX = std::max(x, maxX);
        minY = std::min(y, minY);
        maxY = std::max(y, maxY);
        
        m_edges[i].polygon = this;
        m_edges[i].i1 = i;
        m_edges[i].i2 = (i+1) % nVertices;
        
        sortedEdgesMinY[i] = &m_edges[i];
    }
    
    m_boundingBox.setX(minX);
    m_boundingBox.setY(minY);
    m_boundingBox.setWidth(maxX - minX);
    m_boundingBox.setHeight(maxY - minY);

    std::sort(sortedEdgesMinY.begin(), sortedEdgesMinY.end(), MinEdgeYComparator());
    m_edgeTree = new XSEdgeTree(sortedEdgesMinY);
}

XSPolygon::~XSPolygon() 
{
    if (m_edgeTree)
        delete m_edgeTree;
}

static bool computeXIntersection(const XSEdge* ep, float y, XSIntersection& i)
{
    const XSEdge& e = *ep;
    
    if (e.minY() > y || e.maxY() < y)
        return false;
    
    float x1 = e.x1();
    float y1 = e.y1();
    float x2 = e.x2();
    float y2 = e.y2();
    float dy = y2 - y1;
    
    i.edge = ep;
    i.y = y;
    
    if (dy == 0) {
        i.type = YBOTH;
        i.x = e.minX();
    }
    else if (y == e.minY()) {
        i.type = YMIN;
        i.x = (y1 < y2) ? x1 : x2;
    }
    else if (y == e.maxY()) {
        i.type = YMAX;
        i.x = (y1 > y2) ? x1 : x2;
    }
    else {
        i.type = NORMAL;
        i.x = ((y - y1) * (x2 - x1) / dy) + x1;
    }
    
    return true;
}

float XSPolygon::rightVertexY(size_t index) const 
{ 
    size_t nVertices = numberOfVertices();
    size_t i1 = (index + 1) % nVertices;
    size_t i2 = (index - 1) % nVertices;

    if (getXAt(i1) == getXAt(i2))
        return getYAt(i1) > getYAt(i2) ? getYAt(i1) : getYAt(i2);
    else 
        return getXAt(i1) > getXAt(i2) ? getYAt(i1) : getYAt(i2);
}

static bool appendIntervalX(float x, bool inside, Vector<XSInterval>& v)
{
    if (!inside)
        v.append(XSInterval(x));
    else
        v[v.size() - 1].x2 = x;
    
    return !inside;
}

void XSPolygon::computeXIntersections(float y, Vector<XSInterval>& rv) const
{
    Vector<XSEdge*> overlappingEdges;
    m_edgeTree->findEdgesThatOverlapY(y, overlappingEdges);

    Vector<XSIntersection> intersections;
    XSIntersection intersection;
    for(size_t i = 0; i < overlappingEdges.size(); i++)
    {   
        if (computeXIntersection(overlappingEdges[i], y, intersection) && intersection.type != YBOTH) {
            intersections.append(intersection);
        }
    }
    if (intersections.size() < 2)
        return;
    
    std::sort(intersections.begin(), intersections.end(), IntersectionXComparator());
    
    size_t index = 0;
    int windCount = 0;
    bool inside = false;
    
    while (index < intersections.size())
    {
        const XSIntersection& thisInt = intersections[index];

        if (index + 1 < intersections.size()) {
            const XSIntersection& nextInt = intersections[index +1];
            if ((thisInt.x == nextInt.x) && (thisInt.type == YMIN || thisInt.type == YMAX)) {
                // skip YMAX,YMAX and YMIN,YMIN
                if (thisInt.type == nextInt.type)
                    index += 2;
                else {
                    // YMIN,YMAX or YMAX,YIN => YMIN
                    if (nextInt.type == YMAX)
                        intersections[index + 1] = thisInt;
                    index += 1;
                }
                continue;
            }
        }
        
        const XSEdge& thisEdge = *thisInt.edge;
        bool crossing = windCount == 0; 
        
         if (fillRule() == RULE_EVENODD) {
            windCount += (getYAt(thisEdge.i2) > getYAt(thisEdge.i1)) ? +1 : -1;
            crossing = crossing || windCount == 0;
        }
        
        if ((thisInt.type == NORMAL) || (thisInt.type == YMIN)) {
            if (crossing)
                inside = appendIntervalX(thisInt.x, inside, rv);
        }
        else if (thisInt.type == YMAX) { 
            int vi = (getYAt(thisEdge.i2) > getYAt(thisEdge.i1)) ? thisEdge.i2 : thisEdge.i1;
            if (crossing && rightVertexY(vi) > y)
                inside = appendIntervalX(thisEdge.maxX(), inside, rv);
        }
        
        index += 1;
    }
}

// Return the X projections of the edges that overlap y1,y2.
void XSPolygon::computeEdgeIntersections(float y1, float y2, Vector<XSInterval>& rv) const
{
    if (y1 > y2)
        std::swap(y1, y2);
    
    Vector<XSEdge *> overlappingEdges;
    m_edgeTree->findEdgesThatOverlapInterval(y1, y2, overlappingEdges);
    
    XSIntersection intersection;
    for(size_t i = 0; i < overlappingEdges.size(); i++) {
        const XSEdge& edge = *overlappingEdges[i];
        float x1, x2;
        
        if (edge.minY() < y1) {
            computeXIntersection(overlappingEdges[i], y1, intersection);
            x1 = intersection.x;
        }
        else
            x1 = (edge.y1() < edge.y2()) ? edge.x1() : edge.x2();
        
        if (edge.maxY() > y2) {
            computeXIntersection(overlappingEdges[i], y2, intersection);
            x2 = intersection.x;
        }
        else
            x2 = (edge.y1() > edge.y2()) ? edge.x1() : edge.x2();
        
        if (x1 > x2)      // TBD: Interval constructor should do this
            std::swap(x1, x2);
        
        rv.append(XSInterval(x1, x2));
    }
    
    sortXSIntervals(rv);
}

void XSPolygon::getOutsideIntervals(float y1, float y2, Vector<XSInterval>& rv) const
{
    if (y1 > y2)
        std::swap(y1, y2);
    
    Vector<XSInterval> v1, v2;
    computeXIntersections(y1, v1);
    computeXIntersections(y2, v2);
    
    Vector<XSInterval> mergedIntervals;
    mergeXSIntervals(v1, v2, mergedIntervals);
    
    Vector<XSInterval> edgeIntervals;
    computeEdgeIntersections(y1, y2, edgeIntervals);
    mergeXSIntervals(mergedIntervals, edgeIntervals, rv);
}

void XSPolygon::getInsideIntervals(float y1, float y2, Vector<XSInterval>& rv) const
{
    if (y1 > y2)
        std::swap(y1, y2);
    
    Vector<XSInterval> v1, v2;
    computeXIntersections(y1, v1);
    computeXIntersections(y2, v2);
    
    Vector<XSInterval> commonIntervals;
    intersectXSIntervals(v1, v2, commonIntervals);
    
    Vector<XSInterval> edgeIntervals;
    computeEdgeIntersections(y1, y2, edgeIntervals);
    subtractXSIntervals(commonIntervals, edgeIntervals, rv);
}

} // namespace WebCore

