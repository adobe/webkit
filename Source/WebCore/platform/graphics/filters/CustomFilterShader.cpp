/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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
#include "CustomFilterShader.h"
#include "GraphicsContext3D.h"

#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/RandomNumber.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StripComments.h>

#if PLATFORM(QT)
#include "ANGLE/include/GLSLANG/ShaderLang.h"
#elif !PLATFORM(GTK) && !PLATFORM(EFL) && !PLATFORM(CHROMIUM)
#include "ANGLE/ShaderLang.h"
#else
#include "ShaderLang.h"
#endif

namespace WebCore {

static String randomGeneratedString(size_t len)
{
    const char alphabetSize = 'z' - 'a';
    StringBuilder builder;
    builder.append('_');
    for (size_t i = 0; i < len; ++i) {
        char character = static_cast<char>(trunc(randomNumber() * alphabetSize)) + 'a';
        builder.append(character);
    }
    return builder.toString();
}

CustomFilterShader::CustomFilterShader(GraphicsContext3D* context, const String& vertexShaderString, const String& fragmentShaderString)
    : m_context(context)
    , m_vertexShaderString(vertexShaderString)
    , m_fragmentShaderString(fragmentShaderString)
    , m_program(0)
    , m_positionAttribLocation(-1)
    , m_texAttribLocation(-1)
    , m_meshAttribLocation(-1)
    , m_triangleAttribLocation(-1)
    , m_meshBoxLocation(-1)
    , m_projectionMatrixLocation(-1)
    , m_tileSizeLocation(-1)
    , m_meshSizeLocation(-1)
    , m_samplerLocation(-1)
    , m_samplerSizeLocation(-1)
    , m_contentSamplerLocation(-1)
    , m_isInitialized(false)
{
    ASSERT(!vertexShaderString.isNull());
    ASSERT(!fragmentShaderString.isNull());
    
    m_hiddenSuffix = randomGeneratedString(5);
    
    if (!rewriteShaders()) {
        printf("Shader compiler error: %s\n", m_errorLog.utf8().data());
        return;
    }
    
    Platform3DObject vertexShader = compileShader(GraphicsContext3D::VERTEX_SHADER, m_vertexShaderString);
    if (!vertexShader)
        return;
    
    Platform3DObject fragmentShader = compileShader(GraphicsContext3D::FRAGMENT_SHADER, m_fragmentShaderString);
    if (!fragmentShader) {
        m_context->deleteShader(vertexShader);
        return;
    }
    
    m_program = linkProgram(vertexShader, fragmentShader);
    
    m_context->deleteShader(vertexShader);
    m_context->deleteShader(fragmentShader);
    
    if (!m_program)
        return;
    
    initializeParameterLocations();
    
    m_isInitialized = true;
}

static bool getANGLETranslatedString(ShHandle compiler, String source, String& shaderValidationLog, String& translatedShaderSource)
{
    CString asciiSource = StripComments(source).result().latin1();
    printf("\n=======\nShader before:\n%s\nAfter:\n%s\n", source.utf8().data(), asciiSource.data());
    const char* const shaderSourceStrings[] = { asciiSource.data() };
    bool validateSuccess = ShCompile(compiler, shaderSourceStrings, 1, SH_OBJECT_CODE);
    if (!validateSuccess) {
        int logSize = 0;
        ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &logSize);
        if (logSize > 1) {
            OwnArrayPtr<char> logBuffer = adoptArrayPtr(new char[logSize]);
            if (logBuffer) {
                ShGetInfoLog(compiler, logBuffer.get());
                shaderValidationLog += logBuffer.get();
            }
        }
        return false;
    }
    
    int translationLength = 0;
    ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &translationLength);
    if (translationLength <= 0)
        return false;
    OwnArrayPtr<char> translationBuffer = adoptArrayPtr(new char[translationLength]);
    if (!translationBuffer)
        return false;
    ShGetObjectCode(compiler, translationBuffer.get());

    StringBuilder shaderSourceBuilder;
    shaderSourceBuilder.append("precision mediump float;\n");
    shaderSourceBuilder.append(translationBuffer.get());
    translatedShaderSource = shaderSourceBuilder.toString();

    printf("Translated to:\n%s\n", translatedShaderSource.utf8().data());
    return true;
}

bool CustomFilterShader::rewriteShaders()
{
    ShInitialize();

    ShBuiltInResources resources;
    ShInitBuiltInResources(&resources);
    
    ShHandle vertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, SH_CSS_SHADERS_SPEC, SH_GLSL_OUTPUT, &resources);
    ShHandle fragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, SH_CSS_SHADERS_SPEC, SH_GLSL_OUTPUT, &resources);
    
    bool valid = false;
    
    if (fragmentCompiler && vertexCompiler) {
        CString hiddenSuffixUtf8 = m_hiddenSuffix.utf8();
        ShSetHiddenSymbolSuffix(vertexCompiler, hiddenSuffixUtf8.data());
        ShSetHiddenSymbolSuffix(fragmentCompiler, hiddenSuffixUtf8.data());
        valid = true;
        if (!getANGLETranslatedString(vertexCompiler, m_vertexShaderString, m_errorLog, m_vertexShaderString))
            valid = false;
        if (!getANGLETranslatedString(fragmentCompiler, m_fragmentShaderString, m_errorLog, m_fragmentShaderString))
            valid = false;
    }
    
    if (fragmentCompiler)
        ShDestruct(fragmentCompiler);
    if (vertexCompiler)
        ShDestruct(vertexCompiler);
    
    return valid;
}

Platform3DObject CustomFilterShader::compileShader(GC3Denum shaderType, const String& shaderString)
{
    Platform3DObject shader = m_context->createShader(shaderType);
    m_context->shaderSource(shader, shaderString);
    m_context->compileShader(shader);
    
    int compiled = 0;
    m_context->getShaderiv(shader, GraphicsContext3D::COMPILE_STATUS, &compiled);
    if (!compiled) {
        // FIXME: This is an invalid shader. Throw some errors.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        String errorLog = m_context->getShaderInfoLog(shader);
        printf("Error compiling shader source:%s\n%s\n", shaderString.utf8().data(), errorLog.utf8().data());
        m_context->deleteShader(shader);
        return 0;
    }
    
    return shader;
}

Platform3DObject CustomFilterShader::linkProgram(Platform3DObject vertexShader, Platform3DObject fragmentShader)
{
    Platform3DObject program = m_context->createProgram();
    m_context->attachShader(program, vertexShader);
    m_context->attachShader(program, fragmentShader);
    m_context->linkProgram(program);
    
    int linked = 0;
    m_context->getProgramiv(program, GraphicsContext3D::LINK_STATUS, &linked);
    if (!linked) {
        // FIXME: Invalid vertex/fragment shader combination. Throw some errors here.
        // https://bugs.webkit.org/show_bug.cgi?id=74416
        m_context->deleteProgram(program);
        return 0;
    }
    
    return program;
}

void CustomFilterShader::initializeParameterLocations()
{
    m_positionAttribLocation = m_context->getAttribLocation(m_program, "a_position");
    m_texAttribLocation = m_context->getAttribLocation(m_program, "a_texCoord");
    m_meshAttribLocation = m_context->getAttribLocation(m_program, "a_meshCoord");
    m_triangleAttribLocation = m_context->getAttribLocation(m_program, "a_triangleCoord");
    m_meshBoxLocation = m_context->getUniformLocation(m_program, "u_meshBox");
    m_tileSizeLocation = m_context->getUniformLocation(m_program, "u_tileSize");
    m_meshSizeLocation = m_context->getUniformLocation(m_program, "u_meshSize");
    m_projectionMatrixLocation = m_context->getUniformLocation(m_program, "u_projectionMatrix");
    String uTextureName = String("css_u_texture") + m_hiddenSuffix;
    m_samplerLocation = m_context->getUniformLocation(m_program, uTextureName.latin1().data());
    printf("texture name is %s found at %d\n", uTextureName.latin1().data(), m_samplerLocation);
    m_samplerSizeLocation = m_context->getUniformLocation(m_program, "u_textureSize");
    m_contentSamplerLocation = m_context->getUniformLocation(m_program, "u_contentTexture");
}

int CustomFilterShader::uniformLocationByName(const String& name)
{
    ASSERT(m_isInitialized);
    // FIXME: Improve this by caching the uniform locations.
    return m_context->getUniformLocation(m_program, name);
}
    
CustomFilterShader::~CustomFilterShader()
{
    if (m_program)
        m_context->deleteProgram(m_program);
}

} // namespace WebCore
#endif // ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
