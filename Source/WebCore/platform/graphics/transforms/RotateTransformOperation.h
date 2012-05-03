/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RotateTransformOperation_h
#define RotateTransformOperation_h

#include "Length.h"
#include "LengthFunctions.h"
#include "TransformOperation.h"

namespace WebCore {

class RotateTransformOperation : public TransformOperation {
public:
    static PassRefPtr<RotateTransformOperation> create(double angle, OperationType type)
    {
        return adoptRef(new RotateTransformOperation(0, 0, 1, Length(0, Fixed), Length(0, Fixed), angle, type));
    }

    static PassRefPtr<RotateTransformOperation> create(double angle, const Length& originX, const Length& originY, OperationType type)
    {
        return adoptRef(new RotateTransformOperation(0, 0, 1, originX, originY, angle, type));
    }

    static PassRefPtr<RotateTransformOperation> create(double x, double y, double z, double angle, OperationType type)
    {
        return adoptRef(new RotateTransformOperation(x, y, z, Length(0, Fixed), Length(0, Fixed), angle, type));
    }
    
    static PassRefPtr<RotateTransformOperation> create(double x, double y, double z, const Length& originX, const Length& originY, double angle, OperationType type)
    {
        return adoptRef(new RotateTransformOperation(x, y, z, originX, originY, angle, type));
    }
    
    bool hasOrigin() { return !(m_originX.type() == Fixed && m_originX.isZero() && m_originY.type() == Fixed && m_originY.isZero()); }
    
    double originX(const FloatSize& borderBoxSize) const { return floatValueForLength(m_originX, borderBoxSize.width()); }
    double originY(const FloatSize& borderBoxSize) const { return floatValueForLength(m_originY, borderBoxSize.height()); }

    double x() const { return m_x; }
    double y() const { return m_y; }
    double z() const { return m_z; }
    Length originX() const { return m_originX; }
    Length originY() const { return m_originY; }
    double angle() const { return m_angle; }

    void resetOrigin() {
        m_originX = Length(0, Fixed);
        m_originY = Length(0, Fixed);
    }

private:
    virtual bool isIdentity() const { return m_angle == 0; }

    virtual OperationType getOperationType() const { return m_type; }
    virtual bool isSameType(const TransformOperation& o) const { return o.getOperationType() == m_type; }

    virtual bool operator==(const TransformOperation& o) const
    {
        if (!isSameType(o))
            return false;
        const RotateTransformOperation* r = static_cast<const RotateTransformOperation*>(&o);
        return m_x == r->m_x && m_y == r->m_y && m_z == r->m_z && m_originX == r->m_originX && m_originY == r->m_originY && m_angle == r->m_angle;
    }

    virtual bool apply(TransformationMatrix& transform, const FloatSize& borderBoxSize) const
    {
        float originX = this->originX(borderBoxSize);
        float originY = this->originY(borderBoxSize);
        transform.translate(originX, originY);
        transform.rotate3d(m_x, m_y, m_z, m_angle);
        transform.translate(-originX, -originY);
        return m_originX.type() == Percent || m_originY.type() == Percent;
    }

    virtual PassRefPtr<TransformOperation> blend(const TransformOperation* from, double progress, bool blendToIdentity = false);

    RotateTransformOperation(double x, double y, double z, const Length& originX, const Length& originY, double angle, OperationType type)
        : m_x(x)
        , m_y(y)
        , m_z(z)
        , m_originX(originX)
        , m_originY(originY)
        , m_angle(angle)
        , m_type(type)
    {
        ASSERT(type == ROTATE_X || type == ROTATE_Y || type == ROTATE_Z || type == ROTATE_3D);
    }

    double m_x;
    double m_y;
    double m_z;
    Length m_originX;
    Length m_originY;
    double m_angle;
    OperationType m_type;
};

} // namespace WebCore

#endif // RotateTransformOperation_h
