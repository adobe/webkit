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

#ifndef XS_Rectangle_h
#define XS_Rectangle_h

#include "XShape.h"
#include <wtf/Assertions.h>
#include <wtf/Vector.h>

namespace WebCore {

class XSRectangle : public XShape {
public:
    XSRectangle(float x, float y, float width, float height, float rx = 0, float ry = 0) 
        : XShape()
        , m_x(x)
        , m_y(y)
        , m_width(width)
        , m_height(height)
        , m_rx(rx)
        , m_ry(ry) 
    {
    }
    
    virtual void getOutsideIntervals(float y1, float y2, Vector<XSInterval>&) const;
    virtual void getInsideIntervals(float y1, float y2, Vector<XSInterval>&) const;

private:
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    float m_rx;
    float m_ry;
};

} // namespace WebCore

#endif // XS_Rectangle_h
