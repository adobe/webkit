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

#if ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
#include "CustomFiltersController.h"

#include "CustomFiltersHost.h"
#include "GraphicsContext3D.h"

namespace WebCore {

PassOwnPtr<CustomFiltersController> CustomFiltersController::create(CustomFiltersHost* host)
{
    return adoptPtr(new CustomFiltersController(host));
}

CustomFiltersController::CustomFiltersController(CustomFiltersHost* host)
    : m_host(host)
{
}

CustomFiltersController::~CustomFiltersController()
{
}

bool CustomFiltersController::initializeContext()
{
    GraphicsContext3D::Attributes attributes;
    attributes.preserveDrawingBuffer = true;
    attributes.premultipliedAlpha = false;

    ASSERT(!m_context.get());
    m_context = GraphicsContext3D::create(attributes, m_host->customFiltersHostWindow(), GraphicsContext3D::RenderOffscreen);
    if (!m_context || !m_context->makeContextCurrent()) {
        m_context = 0;
        return false;
    }
    return true;
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
