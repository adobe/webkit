/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
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

#ifndef StyleCustomFilterProgramCache_h
#define StyleCustomFilterProgramCache_h

#if ENABLE(CSS_SHADERS)
#include "KURL.h"
#include "KURLHash.h"
#include "StyleCustomFilterProgram.h"
#include <wtf/HashMap.h>
#include <wtf/FastAllocBase.h>

namespace WebCore {

// CSS Shaders
class CachedShader;

class StyleCustomFilterProgramCache {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StyleCustomFilterProgramCache()
    {
    }

    ~StyleCustomFilterProgramCache() {
        // Make sure the programs are not calling back into this object.
        for (CacheMap::iterator iter = m_cache.begin(), end = m_cache.end(); iter != end; ++iter)
            iter->value->setCache(0);
    }

    StyleCustomFilterProgram* lookup(KURL vertexShader, KURL fragmentShader) const {
        CacheMap::const_iterator iter = m_cache.find(CacheKey(vertexShader, fragmentShader));
        return iter != m_cache.end() ? iter->value : 0;
    }

    void set(StyleCustomFilterProgram* program) {
        CacheKey key = programCacheKey(program);
        ASSERT(m_cache.find(key) == m_cache.end());
        m_cache.set(key, program);
        program->setCache(this);
        ASSERT(m_cache.find(key) != m_cache.end());
    }

    void remove(StyleCustomFilterProgram* program) {
        CacheMap::iterator iter = m_cache.find(programCacheKey(program));
        ASSERT(iter != m_cache.end());
        m_cache.remove(iter);
    }

private:
    typedef pair<KURL, KURL> CacheKey;
    typedef HashMap<CacheKey, StyleCustomFilterProgram*> CacheMap;

    CacheKey programCacheKey(StyleCustomFilterProgram* program) 
    {
        return CacheKey(program->vertexShaderURL(), program->fragmentShaderURL());
    }

    CacheMap m_cache;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // StyleCustomFilterProgramCache_h
