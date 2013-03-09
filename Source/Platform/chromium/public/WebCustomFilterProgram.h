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

#ifndef WebCustomFilterProgram_h
#define WebCustomFilterProgram_h

#include "WebCommon.h"
#include "WebString.h"

namespace WebKit {

class WebCustomFilterProgram {
public:
    // The implementation should make sure that IDs are unique.
    virtual unsigned id() const = 0;
    virtual WebString vertexShader() const = 0;
    virtual WebString fragmentShader() const = 0;

    void ref() { refFromWebCustomFilterProgram(); }
    void deref() { derefFromCustomFilterProgram(); }

    // Use IDs to avoid the whole shader strings.
    bool operator==(const WebCustomFilterProgram& other) const { return id() == other.id(); }
    bool operator!=(const WebCustomFilterProgram& other) const { return !(*this == other); }

private:
    virtual void refFromWebCustomFilterProgram() = 0;
    virtual void derefFromCustomFilterProgram() = 0;
};

} // namespace WebKit

#endif // WebCustomFilterProgram_h

