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

#ifndef WebCustomFilterOperationPrivate_h
#define WebCustomFilterOperationPrivate_h

#include <public/WebFilterOperation.h>
#include <public/WebCustomFilterParameter.h>
#include <public/WebCustomFilterProgram.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebKit {

class WebCustomFilterOperationPrivate: public RefCounted<WebCustomFilterOperationPrivate> {
public:
    static PassRefPtr<WebCustomFilterOperationPrivate> create()
    {
        return adoptRef(new WebCustomFilterOperationPrivate());
    }
    static PassRefPtr<WebCustomFilterOperationPrivate> createCopyWithNoProgram(const WebCustomFilterOperationPrivate& other)
    {
        RefPtr<WebCustomFilterOperationPrivate> filterOperationPrivate = create();
        filterOperationPrivate->meshRows = other.meshRows;
        filterOperationPrivate->meshColumns = other.meshColumns;
        filterOperationPrivate->m_parameters = other.m_parameters;
        return filterOperationPrivate.release();
    }
    ~WebCustomFilterOperationPrivate();

    int meshRows;
    int meshColumns;
    WebCustomFilterMeshType meshType;

    void parameters(WebVector<WebCustomFilterParameter>&) const;
    void setParameters(const WebVector<WebCustomFilterParameter>&);
    void addParameter(const WebCustomFilterParameter&);

    void setProgram(PassRefPtr<WebCustomFilterProgram> program);
    WebCustomFilterProgram* program() const;

    bool operator==(const WebCustomFilterOperationPrivate& other) const;
private:
    WebCustomFilterOperationPrivate();

    RefPtr<WebCustomFilterProgram> m_program;
    Vector<WebCustomFilterParameter> m_parameters;
};


} // namespace WebKit

#endif // WebCustomFilterOperationPrivate_h

