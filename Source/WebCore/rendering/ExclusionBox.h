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

#ifndef ExclusionBox_h
#define ExclusionBox_h

#include "IntRect.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class RenderObject;
class RenderBox;

class ExclusionBox : public RefCounted<ExclusionBox> {
public:
    static PassRefPtr<ExclusionBox> create(RenderBox* box)
    {
        return adoptRef(new ExclusionBox(box)); 
    }

    RenderBox* renderer() const { return m_renderBox; }

    void setBoundingBox(const IntRect& boundingBox) { m_boundingBox = boundingBox; }
    const IntRect& boundingBox() const { return m_boundingBox; }

    void setIsPositioned(bool positioned) { m_isPositioned = positioned; }
    bool isPositioned() const { return m_isPositioned; }

private:
    ExclusionBox(RenderBox* box)
        : m_renderBox(box)
        , m_isPositioned(false)
    {
    }

    RenderBox* m_renderBox;
    IntRect m_boundingBox;
    bool m_isPositioned;
};

} // Namespace WebCore

#endif // WrappingContext_h
