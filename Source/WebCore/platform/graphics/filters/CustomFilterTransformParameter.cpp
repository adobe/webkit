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

#include "config.h"

#if ENABLE(CSS_SHADERS)
#include "CustomFilterTransformParameter.h"

#include "IdentityTransformOperation.h"
#include "Matrix3DTransformOperation.h"

namespace WebCore {

// FIXME: The blending is duplicated from CSSPropertyAnimation.
static bool matchingTransformFunctionList(const TransformOperations* val, const TransformOperations* toVal)
{
    if (val->operations().isEmpty())
        val = toVal;

    if (val->operations().isEmpty())
        return false;

    // An emtpy transform list matches anything.
    if (val != toVal && !toVal->operations().isEmpty() && !val->operationsMatch(*toVal))
        return false;
    
    return true;
}

PassRefPtr<CustomFilterParameter> CustomFilterTransformParameter::blend(const CustomFilterParameter* fromParameter, double progress)
{
    if (!fromParameter || !isSameType(*fromParameter))
        return this;
    const CustomFilterTransformParameter* fromTransformParameter = static_cast<const CustomFilterTransformParameter*>(fromParameter);
    const TransformOperations& from = fromTransformParameter->transform();
    const TransformOperations& to = transform();

    TransformOperations result;

    // If we have a transform function list, use that to do a per-function animation. Otherwise do a Matrix animation
    if (matchingTransformFunctionList(&from, &to)) {
        unsigned fromSize = from.operations().size();
        unsigned toSize = to.operations().size();
        unsigned size = std::max(fromSize, toSize);
        for (unsigned i = 0; i < size; i++) {
            RefPtr<TransformOperation> fromOp = (i < fromSize) ? from.operations()[i].get() : 0;
            RefPtr<TransformOperation> toOp = (i < toSize) ? to.operations()[i].get() : 0;
            RefPtr<TransformOperation> blendedOp = toOp ? toOp->blend(fromOp.get(), progress) : (fromOp ? fromOp->blend(0, progress, true) : PassRefPtr<TransformOperation>(0));
            if (blendedOp)
                result.operations().append(blendedOp);
            else {
                RefPtr<TransformOperation> identityOp = IdentityTransformOperation::create();
                if (progress > 0.5)
                    result.operations().append(toOp ? toOp : identityOp);
                else
                    result.operations().append(fromOp ? fromOp : identityOp);
            }
        }
    } else {
        // Convert the TransformOperations into matrices using a 
        LayoutSize size = LayoutSize(1, 1);
        TransformationMatrix fromT;
        TransformationMatrix toT;
        from.apply(size, fromT);
        to.apply(size, toT);

        toT.blend(fromT, progress);

        // Append the result
        result.operations().append(Matrix3DTransformOperation::create(toT));
    }

    return CustomFilterTransformParameter::create(name(), result);
}
    
bool CustomFilterTransformParameter::operator==(const CustomFilterParameter& o) const
{
    if (!isSameType(o))
        return false;
    const CustomFilterTransformParameter* other = static_cast<const CustomFilterTransformParameter*>(&o);
    return m_transform == other->m_transform;
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)
