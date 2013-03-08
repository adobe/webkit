/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <public/WebFilterOperation.h>
#include <src/WebCustomFilterOperationPrivate.h>

#include <string.h>

namespace WebKit {

bool WebFilterOperation::equals(const WebFilterOperation& other) const
{
    if (m_type != other.m_type)
        return false;
    if (m_type == FilterTypeColorMatrix)
        return !memcmp(m_matrix, other.m_matrix, sizeof(m_matrix));
    if (m_type == FilterTypeDropShadow) {
        return m_amount == other.m_amount
            && m_dropShadowOffset == other.m_dropShadowOffset
            && m_dropShadowColor == other.m_dropShadowColor;
    } else if (m_type == FilterTypeCustom)
        return *m_customFilterPrivate.get() == *other.m_customFilterPrivate.get();
    else
        return m_amount == other.m_amount;
}

WebFilterOperation::WebFilterOperation(FilterType type, SkScalar matrix[20])
{
    WEBKIT_ASSERT(type == FilterTypeColorMatrix);
    m_type = type;
    memcpy(m_matrix, matrix, sizeof(m_matrix));
}

WebFilterOperation::WebFilterOperation(const WebFilterOperation& o)
    : m_type(o.m_type)
    , m_amount(o.m_amount)
    , m_dropShadowOffset(o.m_dropShadowOffset)
    , m_dropShadowColor(o.m_dropShadowColor)
    , m_zoomRect(o.m_zoomRect)
    , m_customFilterPrivate(o.m_customFilterPrivate)
{
    if (m_type == FilterTypeColorMatrix)
        memcpy(m_matrix, o.m_matrix, sizeof(m_matrix));
}

void WebFilterOperation::assign(const WebFilterOperation& o)
{
    m_type = o.m_type;
    m_amount = o.m_amount;
    m_dropShadowOffset = o.m_dropShadowOffset;
    m_dropShadowColor = o.m_dropShadowColor;
    m_zoomRect = o.m_zoomRect;
    m_customFilterPrivate = o.m_customFilterPrivate;
    if (m_type == FilterTypeColorMatrix)
        memcpy(m_matrix, o.m_matrix, sizeof(m_matrix));
}

void WebFilterOperation::destroy()
{
    m_customFilterPrivate.reset();
}

void WebFilterOperation::ensureCustomFilterOperationPrivate()
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    if (!m_customFilterPrivate.get())
        m_customFilterPrivate = WebCustomFilterOperationPrivate::create();
}

WebCustomFilterProgram* WebFilterOperation::customFilterProgram() const
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    return m_customFilterPrivate->program();
}

void WebFilterOperation::setCustomFilterProgram(WebCustomFilterProgram* program)
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate->setProgram(program);
}

void WebFilterOperation::setCustomFilterProgramClone(WebCustomFilterProgram* program)
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate = WebCustomFilterOperationPrivate::createCopyWithNoProgram(*m_customFilterPrivate.get());
    m_customFilterPrivate->setProgram(program);
}

WebCustomFilterMeshType WebFilterOperation::meshType() const
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    return m_customFilterPrivate->meshType;
}

void WebFilterOperation::setMeshType(WebCustomFilterMeshType meshType)
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate->meshType = meshType;
}

int WebFilterOperation::meshRows() const
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    return m_customFilterPrivate->meshRows;
}

void WebFilterOperation::setMeshRows(int meshRows)
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate->meshRows = meshRows;
}

int WebFilterOperation::meshColumns() const
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    return m_customFilterPrivate->meshColumns;
}

void WebFilterOperation::setMeshColumns(int meshColumns)
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate->meshColumns = meshColumns;
}

void WebFilterOperation::customFilterParameters(WebVector<WebCustomFilterParameter>& result) const
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate->parameters(result);
}

void WebFilterOperation::setCustomFilterParameters(const WebVector<WebCustomFilterParameter>& parameters)
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate->setParameters(parameters);
}

void WebFilterOperation::appendCustomFilterParameter(const WebCustomFilterParameter& parameter)
{
    WEBKIT_ASSERT(m_type == FilterTypeCustom);
    m_customFilterPrivate->addParameter(parameter);
}

} // namespace WebKit
