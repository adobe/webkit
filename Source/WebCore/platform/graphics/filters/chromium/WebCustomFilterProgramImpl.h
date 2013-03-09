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

#include "ChromiumPlatformCompiledProgram.h"
#include <public/WebCustomFilterProgram.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class WebCustomFilterProgramImpl: public RefCounted<WebCustomFilterProgramImpl>, public WebKit::WebCustomFilterProgram, public ChromiumPlatformCompiledProgramClient {
public:
    using RefCounted<WebCustomFilterProgramImpl>::ref;
    using RefCounted<WebCustomFilterProgramImpl>::deref;

    static PassRefPtr<WebCustomFilterProgramImpl> create()
    {
        return adoptRef(new WebCustomFilterProgramImpl());
    }
    virtual ~WebCustomFilterProgramImpl();

    virtual unsigned id() const { return m_id; }
    
    virtual WebKit::WebString vertexShader() const;
    virtual void setVertexShader(const WebKit::WebString&);

    virtual WebKit::WebString fragmentShader() const;
    virtual void setFragmentShader(const WebKit::WebString&);

private:
    virtual void refFromWebCustomFilterProgram() { ref(); }
    virtual void derefFromCustomFilterProgram() { deref(); }

    virtual void refFromValidatedProgram() { ref(); }
    virtual void derefFromValidatedProgram() { deref(); } 

    WebCustomFilterProgramImpl();

    unsigned m_id;
    static unsigned s_idGenerator;

    WebKit::WebString m_vertexShader;
    WebKit::WebString m_fragmentShader;
};


} // namespace WebCore

#endif // WebCustomFilterOperationPrivate_h

