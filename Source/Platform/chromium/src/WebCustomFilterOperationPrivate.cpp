/*
 * Copyright (C) 2013 Adobe Systems Inc. All rights reserved.
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
#include <src/WebCustomFilterOperationPrivate.h>

namespace WebKit {

bool operator==(const WebCustomFilterParameter& a, const WebCustomFilterParameter& b)
{
    if (a.type != b.type || a.name != b.name)
        return false;
    switch (a.type) {
    case WebCustomFilterParameter::ParameterTypeNumber:
    case WebCustomFilterParameter::ParameterTypeArray:
        if (a.values.size() != b.values.size())
            return false;
        for (size_t i = 0; i < a.values.size(); ++i)
            if (a.values[i] != b.values[i])
                return false;
        return true;
    case WebCustomFilterParameter::ParameterTypeTransform:
        return a.matrix == b.matrix;
    }
}

bool WebCustomFilterOperationPrivate::operator==(const WebCustomFilterOperationPrivate& other) const
{
    if (meshRows != other.meshRows
        || meshColumns != other.meshColumns
        || meshType != other.meshType
        || m_program.get() != other.m_program.get()
        || m_parameters.size() != other.m_parameters.size())
        return false;
    for (size_t i = 0; i < m_parameters.size(); ++i)
        if (m_parameters[i] != other.m_parameters[i])
            return false;
    return true;
}

WebCustomFilterOperationPrivate::WebCustomFilterOperationPrivate()
    : meshRows(0)
    , meshColumns(0)
    , meshType(WebMeshTypeAttached)
{
}

WebCustomFilterOperationPrivate::~WebCustomFilterOperationPrivate()
{
}

void WebCustomFilterOperationPrivate::setProgram(PassRefPtr<WebCustomFilterProgram> program)
{
    m_program = program;
}

WebCustomFilterProgram* WebCustomFilterOperationPrivate::program() const
{
    return m_program.get();
}

void WebCustomFilterOperationPrivate::parameters(WebVector<WebCustomFilterParameter>& result) const
{
    WebVector<WebCustomFilterParameter> data(m_parameters.size());
    for (unsigned i = 0; i < m_parameters.size(); ++i)
        data[i] = m_parameters[i];
    result.swap(data);
}

void WebCustomFilterOperationPrivate::setParameters(const WebVector<WebCustomFilterParameter>& data)
{
    m_parameters.resize(data.size());
    for (unsigned i = 0; i < data.size(); ++i)
        m_parameters[i] = data[i];
}

void WebCustomFilterOperationPrivate::addParameter(const WebCustomFilterParameter& parameter)
{
    m_parameters.append(parameter);
}

} // namespace WebKit
