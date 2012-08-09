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

#ifndef XS_Polygon_h
#define XS_Polygon_h

#include "XShape.h"
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

struct XSEdge;
struct XSEdgeTree;

class XSPolygon : public XShape {    
public:
    XSPolygon(PassOwnPtr<Vector<float> > coordinates, WindRule fillRule);
    virtual ~XSPolygon();

    float getXAt(size_t i) const { return m_coordinates->at(i*2); }
    float getYAt(size_t i) const { return m_coordinates->at(i*2 + 1); }
    size_t numberOfVertices() const { return m_coordinates->size() / 2; }
    WindRule fillRule() const { return m_fillRule; }

    virtual void getOutsideIntervals(float y1, float y2, Vector<XSInterval>&) const;
    virtual void getInsideIntervals(float y1, float y2, Vector<XSInterval>&) const;

private:
    float rightVertexY(size_t) const;
    void computeXIntersections(float y, Vector<XSInterval>&) const; 
    void computeEdgeIntersections(float yMin, float yMax, Vector<XSInterval>&) const; 

    OwnPtr<Vector<float> > m_coordinates;
    WindRule m_fillRule;
    FloatRect m_boundingBox;
    Vector<XSEdge> m_edges;
    XSEdgeTree* m_edgeTree;
};

struct XSEdge {
    const XSPolygon* polygon;
    size_t i1;
    size_t i2;
    
    XSEdge() 
        : polygon(0)
        , i1(0)
        , i2(0) 
    { 
    }
    
    float x1() const { return polygon->getXAt(i1); }
    float y1() const { return polygon->getYAt(i1); }
    float x2() const { return polygon->getXAt(i2); }
    float y2() const { return polygon->getYAt(i2); }

    float minX() const { return std::min(x1(), x2()); }
    float minY() const { return std::min(y1(), y2()); } 
    float maxX() const { return std::max(x1(), x2()); }
    float maxY() const { return std::max(y1(), y2()); }
};

struct XSEdgeTree {
private:
    // FIXME: minSubtreeSize - don't bother creating subtrees for less than N edges
    float center;
    Vector<XSEdge*> overlapCenter; 
    XSEdgeTree *aboveCenter; 
    XSEdgeTree *belowCenter;
    
    static void initEdgeTree(XSEdgeTree*, const Vector<XSEdge*>& sortedEdges);
    
public:
    XSEdgeTree(const Vector<XSEdge*>& sortedEdges);

    void findEdgesThatOverlapY(float y, Vector<XSEdge*>&) const;
    void findEdgesThatOverlapInterval(float minY, float maxY, Vector<XSEdge*>&) const;
};

enum XSIntersectionType {
    NORMAL = 0,
    YMIN = 1,
    YMAX = 2,
    YBOTH = 3
};

struct XSIntersection {
    const XSEdge* edge;
    float x;
    float y;
    XSIntersectionType type;
    
    XSIntersection() 
        : edge(0)
        , x(0)
        , y(0)
        , type(NORMAL) 
    { 
    }
};

} // namespace WebCore

#endif // XS_Polygon_h
