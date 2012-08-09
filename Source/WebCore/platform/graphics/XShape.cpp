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
#include "LengthFunctions.h"
#include "WrapShapeFunctions.h"
#include "XShape.h"
#include "XSPolygon.h"
#include "XSRectangle.h"
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

static PassRefPtr<XShape> createXSRectangle(float x, float y, float width, float height, float rx, float ry) 
{
    ASSERT(width >= 0 && height >= 0 && rx >= 0 && ry >= 0);
    return adoptRef(new XSRectangle(x, y, width, height, rx, ry));
}

static PassRefPtr<XShape> createXSCircle(float cx, float cy, float radius) 
{
    ASSERT(radius >= 0);
    return adoptRef(new XSRectangle(cx - radius, cy - radius, cx + radius, cy + radius, radius, radius));
}

static PassRefPtr<XShape> createXSEllipse(float cx, float cy, float rx, float ry) 
{
    ASSERT(rx >= 0 && ry >= 0);
    return adoptRef(new XSRectangle(cx - rx, cy - ry, cx + rx, cy + ry, rx, ry));
}

static PassRefPtr<XShape> createXSPolygon(PassOwnPtr<Vector<float> > coordinates, WindRule fillRule)
{
    return adoptRef(new XSPolygon(coordinates, fillRule));
}

PassRefPtr<XShape> XShape::createXShape(const WrapShape *wrapShape, const LayoutRect& borderBox) 
{
    if (!wrapShape)
        return 0;

    const float boxWidth = borderBox.width();
    const float boxHeight = borderBox.height();

    switch(wrapShape->type()) {
        case WrapShape::WRAP_SHAPE_RECTANGLE: {
            const WrapShapeRectangle* rectangle = static_cast<const WrapShapeRectangle*>(wrapShape);
            Length rx = rectangle->cornerRadiusX();
            Length ry = rectangle->cornerRadiusY();
            return createXSRectangle(
                floatValueForLength(rectangle->x(), boxWidth),
                floatValueForLength(rectangle->y(), boxHeight),
                floatValueForLength(rectangle->width(), boxWidth),
                floatValueForLength(rectangle->height(), boxHeight),
                rx.isUndefined() ? 0 : floatValueForLength(rx, boxWidth),
                ry.isUndefined() ? 0 : floatValueForLength(ry, boxHeight) );
        }
           
        case WrapShape::WRAP_SHAPE_CIRCLE: {
            const WrapShapeCircle* circle = static_cast<const WrapShapeCircle*>(wrapShape);
            return createXSCircle(
                floatValueForLength(circle->centerX(), boxWidth),
                floatValueForLength(circle->centerY(), boxHeight),
                floatValueForLength(circle->radius(), std::max(boxHeight, boxWidth)) );
        }

        case WrapShape::WRAP_SHAPE_ELLIPSE: {
            const WrapShapeEllipse* ellipse = static_cast<const WrapShapeEllipse*>(wrapShape);
            return createXSEllipse(
                floatValueForLength(ellipse->centerX(), boxWidth),
                floatValueForLength(ellipse->centerY(), boxHeight),
                floatValueForLength(ellipse->radiusX(), boxWidth),
                floatValueForLength(ellipse->radiusY(), boxHeight) );
        }

        case WrapShape::WRAP_SHAPE_POLYGON: {
            const WrapShapePolygon* polygon = static_cast<const WrapShapePolygon*>(wrapShape);
            const Vector<Length>& values = polygon->values();
            size_t valuesSize = values.size();
            Vector<float> *cv = new Vector<float>(valuesSize);
            for (unsigned i = 0; i < valuesSize; i += 2) {
                cv->at(i) = floatValueForLength(values.at(i), boxWidth);
                cv->at(i+1) = floatValueForLength(values.at(i+1), boxHeight);
            }
            return createXSPolygon(adoptPtr(cv), polygon->windRule());
        }

        default:
            ASSERT_NOT_REACHED();
            return 0;
    }

    return 0;
}

} // namespace WebCore
