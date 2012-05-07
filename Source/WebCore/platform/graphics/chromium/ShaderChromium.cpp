/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Adobe Systems, Inc. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING)

#include "ShaderChromium.h"

#include "GraphicsContext.h"
#include "GraphicsContext3D.h"

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/StringBuilder.h>

#define SHADER0(Src) #Src
#define SHADER(Src) SHADER0(Src)

namespace WebCore {

// The following methods are building the shaders for the blend modes explained in CSS Compositing and Blending.
// https://dvcs.w3.org/hg/FXTF/raw-file/782834d5c66c/compositing/index.html

static void blendComponents(StringBuilder& builder, const char* formula)
{
    builder.append("float blendColorComponents(float backgroundColor, float sourceColor) {");
    builder.append(formula);
    builder.append("}");
    builder.append("vec3 blendColors(vec3 backgroundColor, vec3 sourceColor) {");
    builder.append("    return vec3(blendColorComponents(backgroundColor.r, sourceColor.r), blendColorComponents(backgroundColor.g, sourceColor.g), blendColorComponents(backgroundColor.b, sourceColor.b));");
    builder.append("}");
}

static void blendColors(StringBuilder& builder, const char* formula)
{
    builder.append("vec3 blendColors(vec3 backgroundColor, vec3 sourceColor) {");
    builder.append(formula);
    builder.append("}");
}


static void nonSeparableBlendModesHelper(StringBuilder& builder, const char* formula)
{
    builder.append(
        SHADER(
            float sat(vec3 c) {
                float cMin = min(min(c.r, c.g), c.b);
                float cMax = max(max(c.r, c.g), c.b);
                return cMax - cMin;
            }

            float lum(vec3 c) {
                return 0.3 * c.r + 0.59 * c.g + 0.11 * c.b;
            }

            vec3 clipColor(vec3 c) {
                float l = lum(c);
                float n = min(min(c.r, c.g), c.b);
                float x = max(max(c.r, c.g), c.b);
                if (n < 0.0)
                    c = l + (((c - l) * l) / (l - n));
                if (x > 1.0)
                    c = l + (((c - l) * (1.0 - l) / (x - l)));
                return c;
            }

            vec3 setLum(vec3 c, float l) {
                c += l - lum(c);
                return clipColor(c);
            }

            void setSatHelper(inout float cMin, inout float cMid, inout float cMax, float s) {
                if(cMax > cMin) {
                    cMid = (((cMid - cMin) * s) / (cMax - cMin));
                    cMax = s;
                } else {
                    cMid = cMax = 0.0;
                }
                cMin = 0.0;
            }

            vec3 setSat(vec3 c, float s) {
                if (c.r <= c.g) {
                    if (c.g <= c.b)
                        setSatHelper(c.r, c.g, c.b, s); // r <= g <= b
                    else {
                        if (c.r <= c.b)
                            setSatHelper(c.r, c.b, c.g, s); // r <= b <= g
                        else
                            setSatHelper(c.b, c.r, c.g, s); // // b <= r <= g
                    }
                } else {
                    if (c.r <= c.b)
                        setSatHelper(c.g, c.r, c.b, s); // g <= r <= b
                    else {
                        if (c.g <= c.b)
                            setSatHelper(c.g, c.b, c.r, s); // g <= b <= r
                        else 
                            setSatHelper(c.b, c.g, c.r, s); // // b <= g <= r
                    }
                }
                return c;
            }
        )
    );
    blendColors(builder, formula);
}

// The result of the method expects to have a method called blend, that will do the blending.
// vec4 blend(vec4 sourceColor)
static String formulaForBlendMode(EBlendMode blendMode)
{
    StringBuilder builder;
    builder.append("uniform sampler2D s_backgroundTexture;");
    builder.append("uniform vec4 backgroundRect;");
    switch (blendMode) {
        case BlendModeNormal:
            blendColors(builder, "return sourceColor;");
            break;
        case BlendModePlus:
            blendColors(builder, "return sourceColor + backgroundColor;");
            break;
        case BlendModeMultiply:
            blendColors(builder, "return sourceColor * backgroundColor;");
            break;
        case BlendModeScreen:
            blendColors(builder, "return backgroundColor + sourceColor - (backgroundColor * sourceColor);");
            break;
        case BlendModeDifference:
            blendColors(builder, "return abs(backgroundColor - sourceColor);");
            break;
        case BlendModeExclusion:
            blendColors(builder, "return backgroundColor + sourceColor - 2.0 * backgroundColor * sourceColor;");
            break;
        case BlendModeDarken:
            blendColors(builder, "return min(sourceColor, backgroundColor);");
            break;
        case BlendModeLighten:
            blendColors(builder, "return max(sourceColor, backgroundColor);");
            break;
        case BlendModeColorDodge:
            blendComponents(builder, "return (sourceColor == 1.0) ? 0.0 : min(1.0, backgroundColor / (1.0 - sourceColor));");
            break;
        case BlendModeColorBurn:
            blendComponents(builder, "return (sourceColor == 0.0) ? 0.0 : (1.0 - min(1.0, (1.0 - backgroundColor) / sourceColor));");
            break;
        case BlendModeOverlay:
            blendComponents(builder, SHADER(
                if (backgroundColor <= 0.5)
                    return sourceColor * 2.0 * backgroundColor;
                backgroundColor = 2.0 * backgroundColor - 1.0;
                return sourceColor + backgroundColor - (sourceColor * backgroundColor);
            ));
            break;
        case BlendModeHardLight:
            blendComponents(builder, SHADER(
                if (sourceColor <= 0.5)
                    return backgroundColor * 2.0 * sourceColor;
                sourceColor = 2.0 * sourceColor - 1.0;
                return backgroundColor + sourceColor - (backgroundColor * sourceColor);
            ));
            break;
        case BlendModeSoftLight:
            blendComponents(builder, SHADER(
                if (sourceColor <= 0.5)
                    return backgroundColor - (1.0 - 2.0 * sourceColor) * backgroundColor * (1.0 - backgroundColor);
                float D = (backgroundColor <= 0.25)
                            ? ((16.0 * backgroundColor - 12.0) * backgroundColor + 4.0) * backgroundColor 
                            : sqrt(backgroundColor);
                return backgroundColor + (2.0 * sourceColor - 1.0) * (D - backgroundColor);
            ));
            break;
        case BlendModeHue:
            nonSeparableBlendModesHelper(builder, "return setLum(setSat(sourceColor, sat(backgroundColor)), lum(backgroundColor));");
            break;
        case BlendModeSaturation:
            nonSeparableBlendModesHelper(builder, "return setLum(setSat(backgroundColor, sat(sourceColor)), lum(backgroundColor));");
            break;
        case BlendModeColor:
            nonSeparableBlendModesHelper(builder, "return setLum(sourceColor, lum(backgroundColor));");
            break;
        case BlendModeLuminosity:
            nonSeparableBlendModesHelper(builder, "return setLum(backgroundColor, lum(sourceColor));");
            break;
    }
    builder.append(
        SHADER(
            vec4 unmultiply(vec4 premultipliedSourceColor) {
                return vec4(clamp(premultipliedSourceColor.rgb / premultipliedSourceColor.a, 0.0, 1.0), premultipliedSourceColor.a);
            }
            vec4 composite(float alphaA, float Fa, vec3 Ca, 
                           float alphaB, float Fb, vec3 Cb) {
                return clamp(vec4(alphaA * Fa * Ca + alphaB * Fb * Cb, 
                                  alphaA * Fa + alphaB * Fb), 0.0, 1.0);
            }
            vec4 blend(vec4 sourceColor) {
                vec2 bgTexCoord = gl_FragCoord.xy - backgroundRect.xy;
                bgTexCoord.x /= backgroundRect.b; 
                bgTexCoord.y /= backgroundRect.a;
                vec4 backgroundColor = (texture2D(s_backgroundTexture, bgTexCoord));
                float Fa = 1.0;
                float Fb = 1.0 - sourceColor.a;
                return composite(sourceColor.a, Fa, blendColors(backgroundColor.rgb, sourceColor.rgb),
                                 backgroundColor.a, Fb, backgroundColor.rgb);
            }
        )
    );
    return builder.toString();
}

VertexShaderPosTex::VertexShaderPosTex()
    : m_matrixLocation(-1)
{
}

void VertexShaderPosTex::init(GraphicsContext3D* context, unsigned program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    ASSERT(m_matrixLocation != -1);
}

String VertexShaderPosTex::getShaderString() const
{
    return SHADER(
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        uniform mat4 matrix;
        varying vec2 v_texCoord;
        void main()
        {
            gl_Position = matrix * a_position;
            v_texCoord = a_texCoord;
        }
    );
}

VertexShaderPosTexYUVStretch::VertexShaderPosTexYUVStretch()
    : m_matrixLocation(-1)
    , m_yWidthScaleFactorLocation(-1)
    , m_uvWidthScaleFactorLocation(-1)
{
}

void VertexShaderPosTexYUVStretch::init(GraphicsContext3D* context, unsigned program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    m_yWidthScaleFactorLocation = context->getUniformLocation(program, "y_widthScaleFactor");
    m_uvWidthScaleFactorLocation = context->getUniformLocation(program, "uv_widthScaleFactor");
    ASSERT(m_matrixLocation != -1 && m_yWidthScaleFactorLocation != -1 && m_uvWidthScaleFactorLocation != -1);
}

String VertexShaderPosTexYUVStretch::getShaderString() const
{
    return SHADER(
        precision mediump float;
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        uniform mat4 matrix;
        varying vec2 y_texCoord;
        varying vec2 uv_texCoord;
        uniform float y_widthScaleFactor;
        uniform float uv_widthScaleFactor;
        void main()
        {
            gl_Position = matrix * a_position;
            y_texCoord = vec2(y_widthScaleFactor * a_texCoord.x, a_texCoord.y);
            uv_texCoord = vec2(uv_widthScaleFactor * a_texCoord.x, a_texCoord.y);
        }
    );
}

VertexShaderPos::VertexShaderPos()
    : m_matrixLocation(-1)
{
}

void VertexShaderPos::init(GraphicsContext3D* context, unsigned program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    ASSERT(m_matrixLocation != -1);
}

String VertexShaderPos::getShaderString() const
{
    return SHADER(
        attribute vec4 a_position;
        uniform mat4 matrix;
        void main()
        {
            gl_Position = matrix * a_position;
        }
    );
}

VertexShaderPosTexTransform::VertexShaderPosTexTransform()
    : m_matrixLocation(-1)
    , m_texTransformLocation(-1)
{
}

void VertexShaderPosTexTransform::init(GraphicsContext3D* context, unsigned program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    m_texTransformLocation = context->getUniformLocation(program, "texTransform");
    ASSERT(m_matrixLocation != -1 && m_texTransformLocation != -1);
}

String VertexShaderPosTexTransform::getShaderString() const
{
    return SHADER(
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        uniform mat4 matrix;
        uniform vec4 texTransform;
        varying vec2 v_texCoord;
        void main()
        {
            gl_Position = matrix * a_position;
            v_texCoord = a_texCoord * texTransform.zw + texTransform.xy;
        }
    );
}

VertexShaderQuad::VertexShaderQuad()
    : m_matrixLocation(-1)
    , m_pointLocation(-1)
{
}

String VertexShaderPosTexIdentity::getShaderString() const
{
    return SHADER(
        attribute vec4 a_position;
        varying vec2 v_texCoord;
        void main()
        {
            gl_Position = a_position;
            v_texCoord = (a_position.xy + vec2(1.0)) * 0.5;
        }
    );
}

void VertexShaderQuad::init(GraphicsContext3D* context, unsigned program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    m_pointLocation = context->getUniformLocation(program, "point");
    ASSERT(m_matrixLocation != -1 && m_pointLocation != -1);
}

String VertexShaderQuad::getShaderString() const
{
    return SHADER(
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        uniform mat4 matrix;
        uniform vec2 point[4];
        varying vec2 v_texCoord;
        void main()
        {
            vec2 complement = abs(a_texCoord - 1.0);
            vec4 pos = vec4(0.0, 0.0, a_position.z, a_position.w);
            pos.xy += (complement.x * complement.y) * point[0];
            pos.xy += (a_texCoord.x * complement.y) * point[1];
            pos.xy += (a_texCoord.x * a_texCoord.y) * point[2];
            pos.xy += (complement.x * a_texCoord.y) * point[3];
            gl_Position = matrix * pos;
            v_texCoord = pos.xy + vec2(0.5);
        }
    );
}

VertexShaderTile::VertexShaderTile()
    : m_matrixLocation(-1)
    , m_pointLocation(-1)
    , m_vertexTexTransformLocation(-1)
{
}

void VertexShaderTile::init(GraphicsContext3D* context, unsigned program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    m_pointLocation = context->getUniformLocation(program, "point");
    m_vertexTexTransformLocation = context->getUniformLocation(program, "vertexTexTransform");
    ASSERT(m_matrixLocation != -1 && m_pointLocation != -1 && m_vertexTexTransformLocation != -1);
}

String VertexShaderTile::getShaderString() const
{
    return SHADER(
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        uniform mat4 matrix;
        uniform vec2 point[4];
        uniform vec4 vertexTexTransform;
        varying vec2 v_texCoord;
        void main()
        {
            vec2 complement = abs(a_texCoord - 1.0);
            vec4 pos = vec4(0.0, 0.0, a_position.z, a_position.w);
            pos.xy += (complement.x * complement.y) * point[0];
            pos.xy += (a_texCoord.x * complement.y) * point[1];
            pos.xy += (a_texCoord.x * a_texCoord.y) * point[2];
            pos.xy += (complement.x * a_texCoord.y) * point[3];
            gl_Position = matrix * pos;
            v_texCoord = pos.xy * vertexTexTransform.zw + vertexTexTransform.xy;
        }
    );
}

VertexShaderVideoTransform::VertexShaderVideoTransform()
    : m_matrixLocation(-1)
    , m_texTransformLocation(-1)
    , m_texMatrixLocation(-1)
{
}

bool VertexShaderVideoTransform::init(GraphicsContext3D* context, unsigned program)
{
    m_matrixLocation = context->getUniformLocation(program, "matrix");
    m_texTransformLocation = context->getUniformLocation(program, "texTransform");
    m_texMatrixLocation = context->getUniformLocation(program, "texMatrix");
    return m_matrixLocation != -1 && m_texTransformLocation != -1 && m_texMatrixLocation != -1;
}

String VertexShaderVideoTransform::getShaderString() const
{
    return SHADER(
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        uniform mat4 matrix;
        uniform vec4 texTransform;
        uniform mat4 texMatrix;
        varying vec2 v_texCoord;
        void main()
        {
            gl_Position = matrix * a_position;
            vec2 texCoord = vec2(texMatrix * vec4(a_texCoord.x, 1.0 - a_texCoord.y, 0.0, 1.0));
            v_texCoord = texCoord * texTransform.zw + texTransform.xy;
        }
    );
}

FragmentTexAlphaBinding::FragmentTexAlphaBinding()
    : m_samplerLocation(-1)
    , m_backgroundSamplerLocation(-1)
    , m_backgroundRectLocation(-1)
    , m_alphaLocation(-1)
{
}

void FragmentTexAlphaBinding::init(GraphicsContext3D* context, unsigned program, bool needsBackgroundTexture)
{
    m_samplerLocation = context->getUniformLocation(program, "s_texture");
    m_alphaLocation = context->getUniformLocation(program, "alpha");

    ASSERT(m_samplerLocation != -1 && m_alphaLocation != -1);
    
    if (needsBackgroundTexture) {
        m_backgroundSamplerLocation = context->getUniformLocation(program, "s_backgroundTexture");
        m_backgroundRectLocation = context->getUniformLocation(program, "backgroundRect");
        ASSERT(m_backgroundSamplerLocation != -1 && m_backgroundRectLocation != -1);
    }
}

FragmentTexOpaqueBinding::FragmentTexOpaqueBinding()
    : m_samplerLocation(-1)
{
}

void FragmentTexOpaqueBinding::init(GraphicsContext3D* context, unsigned program)
{
    m_samplerLocation = context->getUniformLocation(program, "s_texture");

    ASSERT(m_samplerLocation != -1);
}

String FragmentShaderRGBATexFlipAlpha::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        void main()
        {
            vec4 texColor = texture2D(s_texture, vec2(v_texCoord.x, 1.0 - v_texCoord.y));
            gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha;
        }
    );
}

bool FragmentShaderOESImageExternal::init(GraphicsContext3D* context, unsigned program)
{
    m_samplerLocation = context->getUniformLocation(program, "s_texture");

    return m_samplerLocation != -1;
}

String FragmentShaderOESImageExternal::getShaderString() const
{
    // Cannot use the SHADER() macro because of the '#' char
    return "#extension GL_OES_EGL_image_external : require \n"
           "precision mediump float;\n"
           "varying vec2 v_texCoord;\n"
           "uniform samplerExternalOES s_texture;\n"
           "void main()\n"
           "{\n"
           "    vec4 texColor = texture2D(s_texture, v_texCoord);\n"
           "    gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w);\n"
           "}\n";
}

String FragmentShaderRGBATexAlpha::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            gl_FragColor = texColor * alpha;
        }
    );
}

String FragmentShaderRGBATexAlpha::getShaderString(EBlendMode blendMode) const
{
    String blending = formulaForBlendMode(blendMode);
    return String::format(SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        %s
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            gl_FragColor = blend(texColor * alpha);
        }
    ), blending.latin1().data());
}

String FragmentShaderRGBATexRectFlipAlpha::getShaderString() const
{
    // This must be paired with VertexShaderPosTexTransform to pick up the texTransform uniform.
    // The necessary #extension preprocessing directive breaks the SHADER and SHADER0 macros.
    return "#extension GL_ARB_texture_rectangle : require\n"
            "precision mediump float;\n"
            "varying vec2 v_texCoord;\n"
            "uniform vec4 texTransform;\n"
            "uniform sampler2DRect s_texture;\n"
            "uniform float alpha;\n"
            "void main()\n"
            "{\n"
            "    vec4 texColor = texture2DRect(s_texture, vec2(v_texCoord.x, texTransform.w - v_texCoord.y));\n"
            "    gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha;\n"
            "}\n";
}

String FragmentShaderRGBATexRectAlpha::getShaderString() const
{
    return "#extension GL_ARB_texture_rectangle : require\n"
            "precision mediump float;\n"
            "varying vec2 v_texCoord;\n"
            "uniform sampler2DRect s_texture;\n"
            "uniform float alpha;\n"
            "void main()\n"
            "{\n"
            "    vec4 texColor = texture2DRect(s_texture, v_texCoord);\n"
            "    gl_FragColor = texColor * alpha;\n"
            "}\n";
}

String FragmentShaderRGBATexOpaque::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            gl_FragColor = vec4(texColor.rgb, 1.0);
        }
    );
}

String FragmentShaderRGBATex::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        void main()
        {
            gl_FragColor = texture2D(s_texture, v_texCoord);
        }
    );
}

String FragmentShaderRGBATexSwizzleAlpha::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            gl_FragColor = vec4(texColor.z, texColor.y, texColor.x, texColor.w) * alpha;
        }
    );
}

String FragmentShaderRGBATexSwizzleOpaque::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            gl_FragColor = vec4(texColor.z, texColor.y, texColor.x, 1.0);
        }
    );
}

FragmentShaderRGBATexAlphaAA::FragmentShaderRGBATexAlphaAA()
    : m_samplerLocation(-1)
    , m_backgroundSamplerLocation(-1)
    , m_backgroundRectLocation(-1)
    , m_alphaLocation(-1)
    , m_edgeLocation(-1)
{
}

void FragmentShaderRGBATexAlphaAA::init(GraphicsContext3D* context, unsigned program, bool needsBackgroundTexture)
{
    m_samplerLocation = context->getUniformLocation(program, "s_texture");
    m_alphaLocation = context->getUniformLocation(program, "alpha");
    m_edgeLocation = context->getUniformLocation(program, "edge");

    ASSERT(m_samplerLocation != -1 && m_alphaLocation != -1 && m_edgeLocation != -1);
    
    if (needsBackgroundTexture) {
        m_backgroundSamplerLocation = context->getUniformLocation(program, "s_backgroundTexture");
        m_backgroundRectLocation = context->getUniformLocation(program, "backgroundRect");
        ASSERT(m_backgroundSamplerLocation != -1 && m_backgroundRectLocation != -1);
    }
}

String FragmentShaderRGBATexAlphaAA::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        uniform vec3 edge[8];
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            vec3 pos = vec3(gl_FragCoord.xy, 1);
            float a0 = clamp(dot(edge[0], pos), 0.0, 1.0);
            float a1 = clamp(dot(edge[1], pos), 0.0, 1.0);
            float a2 = clamp(dot(edge[2], pos), 0.0, 1.0);
            float a3 = clamp(dot(edge[3], pos), 0.0, 1.0);
            float a4 = clamp(dot(edge[4], pos), 0.0, 1.0);
            float a5 = clamp(dot(edge[5], pos), 0.0, 1.0);
            float a6 = clamp(dot(edge[6], pos), 0.0, 1.0);
            float a7 = clamp(dot(edge[7], pos), 0.0, 1.0);
            gl_FragColor = texColor * alpha * min(min(a0, a2) * min(a1, a3), min(a4, a6) * min(a5, a7));
        }
    );
}

String FragmentShaderRGBATexAlphaAA::getShaderString(EBlendMode blendMode) const
{
    String blending = formulaForBlendMode(blendMode);
    return String::format(SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        uniform vec3 edge[8];
        %s
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            vec3 pos = vec3(gl_FragCoord.xy, 1);
            float a0 = clamp(dot(edge[0], pos), 0.0, 1.0);
            float a1 = clamp(dot(edge[1], pos), 0.0, 1.0);
            float a2 = clamp(dot(edge[2], pos), 0.0, 1.0);
            float a3 = clamp(dot(edge[3], pos), 0.0, 1.0);
            float a4 = clamp(dot(edge[4], pos), 0.0, 1.0);
            float a5 = clamp(dot(edge[5], pos), 0.0, 1.0);
            float a6 = clamp(dot(edge[6], pos), 0.0, 1.0);
            float a7 = clamp(dot(edge[7], pos), 0.0, 1.0);
            gl_FragColor = blend(texColor * alpha * min(min(a0, a2) * min(a1, a3), min(a4, a6) * min(a5, a7)));
        }
    ), blending.latin1().data());
}

FragmentTexClampAlphaAABinding::FragmentTexClampAlphaAABinding()
    : m_samplerLocation(-1)
    , m_alphaLocation(-1)
    , m_fragmentTexTransformLocation(-1)
    , m_edgeLocation(-1)
{
}

void FragmentTexClampAlphaAABinding::init(GraphicsContext3D* context, unsigned program)
{
    m_samplerLocation = context->getUniformLocation(program, "s_texture");
    m_alphaLocation = context->getUniformLocation(program, "alpha");
    m_fragmentTexTransformLocation = context->getUniformLocation(program, "fragmentTexTransform");
    m_edgeLocation = context->getUniformLocation(program, "edge");

    ASSERT(m_samplerLocation != -1 && m_alphaLocation != -1 && m_fragmentTexTransformLocation != -1 && m_edgeLocation != -1);
}

String FragmentShaderRGBATexClampAlphaAA::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        uniform vec4 fragmentTexTransform;
        uniform vec3 edge[8];
        void main()
        {
            vec2 texCoord = clamp(v_texCoord, 0.0, 1.0) * fragmentTexTransform.zw + fragmentTexTransform.xy;
            vec4 texColor = texture2D(s_texture, texCoord);
            vec3 pos = vec3(gl_FragCoord.xy, 1);
            float a0 = clamp(dot(edge[0], pos), 0.0, 1.0);
            float a1 = clamp(dot(edge[1], pos), 0.0, 1.0);
            float a2 = clamp(dot(edge[2], pos), 0.0, 1.0);
            float a3 = clamp(dot(edge[3], pos), 0.0, 1.0);
            float a4 = clamp(dot(edge[4], pos), 0.0, 1.0);
            float a5 = clamp(dot(edge[5], pos), 0.0, 1.0);
            float a6 = clamp(dot(edge[6], pos), 0.0, 1.0);
            float a7 = clamp(dot(edge[7], pos), 0.0, 1.0);
            gl_FragColor = texColor * alpha * min(min(a0, a2) * min(a1, a3), min(a4, a6) * min(a5, a7));
        }
    );
}

String FragmentShaderRGBATexClampSwizzleAlphaAA::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform float alpha;
        uniform vec4 fragmentTexTransform;
        uniform vec3 edge[8];
        void main()
        {
            vec2 texCoord = clamp(v_texCoord, 0.0, 1.0) * fragmentTexTransform.zw + fragmentTexTransform.xy;
            vec4 texColor = texture2D(s_texture, texCoord);
            vec3 pos = vec3(gl_FragCoord.xy, 1);
            float a0 = clamp(dot(edge[0], pos), 0.0, 1.0);
            float a1 = clamp(dot(edge[1], pos), 0.0, 1.0);
            float a2 = clamp(dot(edge[2], pos), 0.0, 1.0);
            float a3 = clamp(dot(edge[3], pos), 0.0, 1.0);
            float a4 = clamp(dot(edge[4], pos), 0.0, 1.0);
            float a5 = clamp(dot(edge[5], pos), 0.0, 1.0);
            float a6 = clamp(dot(edge[6], pos), 0.0, 1.0);
            float a7 = clamp(dot(edge[7], pos), 0.0, 1.0);
            gl_FragColor = vec4(texColor.z, texColor.y, texColor.x, texColor.w) * alpha * min(min(a0, a2) * min(a1, a3), min(a4, a6) * min(a5, a7));
        }
    );
}

FragmentShaderRGBATexAlphaMask::FragmentShaderRGBATexAlphaMask()
    : m_samplerLocation(-1)
    , m_backgroundSamplerLocation(-1)
    , m_backgroundRectLocation(-1)
    , m_maskSamplerLocation(-1)
    , m_alphaLocation(-1)
{
}

void FragmentShaderRGBATexAlphaMask::init(GraphicsContext3D* context, unsigned program, bool needsBackgroundTexture)
{
    m_samplerLocation = context->getUniformLocation(program, "s_texture");
    m_maskSamplerLocation = context->getUniformLocation(program, "s_mask");
    m_alphaLocation = context->getUniformLocation(program, "alpha");
    ASSERT(m_samplerLocation != -1 && m_maskSamplerLocation != -1 && m_alphaLocation != -1);
    
    if (needsBackgroundTexture) {
        m_backgroundSamplerLocation = context->getUniformLocation(program, "s_backgroundTexture");
        m_backgroundRectLocation = context->getUniformLocation(program, "backgroundRect");
        ASSERT(m_backgroundSamplerLocation != -1 && m_backgroundRectLocation != -1);
    }
}

String FragmentShaderRGBATexAlphaMask::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform sampler2D s_mask;
        uniform float alpha;
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            vec4 maskColor = texture2D(s_mask, v_texCoord);
            gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha * maskColor.w;
        }
    );
}

String FragmentShaderRGBATexAlphaMask::getShaderString(EBlendMode blendMode) const
{
    String blending = formulaForBlendMode(blendMode);
    return String::format(SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform sampler2D s_mask;
        uniform float alpha;
        %s
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            vec4 maskColor = texture2D(s_mask, v_texCoord);
            gl_FragColor = blend(vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha * maskColor.w);
        }
    ), blending.latin1().data());
}

FragmentShaderRGBATexAlphaMaskAA::FragmentShaderRGBATexAlphaMaskAA()
    : m_samplerLocation(-1)
    , m_backgroundSamplerLocation(-1)
    , m_backgroundRectLocation(-1)
    , m_maskSamplerLocation(-1)
    , m_alphaLocation(-1)
    , m_edgeLocation(-1)
{
}

void FragmentShaderRGBATexAlphaMaskAA::init(GraphicsContext3D* context, unsigned program, bool needsBackgroundTexture)
{
    m_samplerLocation = context->getUniformLocation(program, "s_texture");
    m_maskSamplerLocation = context->getUniformLocation(program, "s_mask");
    m_alphaLocation = context->getUniformLocation(program, "alpha");
    m_edgeLocation = context->getUniformLocation(program, "edge");
    ASSERT(m_samplerLocation != -1 && m_maskSamplerLocation != -1 && m_alphaLocation != -1 && m_edgeLocation != -1);
    
    if (needsBackgroundTexture) {
        m_backgroundSamplerLocation = context->getUniformLocation(program, "s_backgroundTexture");
        m_backgroundRectLocation = context->getUniformLocation(program, "backgroundRect");
        ASSERT(m_backgroundSamplerLocation != -1 && m_backgroundRectLocation != -1);
    }
}

String FragmentShaderRGBATexAlphaMaskAA::getShaderString() const
{
    return SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform sampler2D s_mask;
        uniform float alpha;
        uniform vec3 edge[8];
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            vec4 maskColor = texture2D(s_mask, v_texCoord);
            vec3 pos = vec3(gl_FragCoord.xy, 1);
            float a0 = clamp(dot(edge[0], pos), 0.0, 1.0);
            float a1 = clamp(dot(edge[1], pos), 0.0, 1.0);
            float a2 = clamp(dot(edge[2], pos), 0.0, 1.0);
            float a3 = clamp(dot(edge[3], pos), 0.0, 1.0);
            float a4 = clamp(dot(edge[4], pos), 0.0, 1.0);
            float a5 = clamp(dot(edge[5], pos), 0.0, 1.0);
            float a6 = clamp(dot(edge[6], pos), 0.0, 1.0);
            float a7 = clamp(dot(edge[7], pos), 0.0, 1.0);
            gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha * maskColor.w * min(min(a0, a2) * min(a1, a3), min(a4, a6) * min(a5, a7));
        }
    );
}

String FragmentShaderRGBATexAlphaMaskAA::getShaderString(EBlendMode blendMode) const
{
    String blending = formulaForBlendMode(blendMode);
    return String::format(SHADER(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        uniform sampler2D s_mask;
        uniform float alpha;
        uniform vec3 edge[8];
        %s
        void main()
        {
            vec4 texColor = texture2D(s_texture, v_texCoord);
            vec4 maskColor = texture2D(s_mask, v_texCoord);
            vec3 pos = vec3(gl_FragCoord.xy, 1);
            float a0 = clamp(dot(edge[0], pos), 0.0, 1.0);
            float a1 = clamp(dot(edge[1], pos), 0.0, 1.0);
            float a2 = clamp(dot(edge[2], pos), 0.0, 1.0);
            float a3 = clamp(dot(edge[3], pos), 0.0, 1.0);
            float a4 = clamp(dot(edge[4], pos), 0.0, 1.0);
            float a5 = clamp(dot(edge[5], pos), 0.0, 1.0);
            float a6 = clamp(dot(edge[6], pos), 0.0, 1.0);
            float a7 = clamp(dot(edge[7], pos), 0.0, 1.0);
            gl_FragColor = blend(vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha * maskColor.w * min(min(a0, a2) * min(a1, a3), min(a4, a6) * min(a5, a7)));
        }
    ), blending.latin1().data());
}

FragmentShaderYUVVideo::FragmentShaderYUVVideo()
    : m_yTextureLocation(-1)
    , m_uTextureLocation(-1)
    , m_vTextureLocation(-1)
    , m_alphaLocation(-1)
    , m_ccMatrixLocation(-1)
    , m_yuvAdjLocation(-1)
{
}

void FragmentShaderYUVVideo::init(GraphicsContext3D* context, unsigned program)
{
    m_yTextureLocation = context->getUniformLocation(program, "y_texture");
    m_uTextureLocation = context->getUniformLocation(program, "u_texture");
    m_vTextureLocation = context->getUniformLocation(program, "v_texture");
    m_alphaLocation = context->getUniformLocation(program, "alpha");
    m_ccMatrixLocation = context->getUniformLocation(program, "cc_matrix");
    m_yuvAdjLocation = context->getUniformLocation(program, "yuv_adj");

    ASSERT(m_yTextureLocation != -1 && m_uTextureLocation != -1 && m_vTextureLocation != -1
           && m_alphaLocation != -1 && m_ccMatrixLocation != -1 && m_yuvAdjLocation != -1);
}

String FragmentShaderYUVVideo::getShaderString() const
{
    return SHADER(
        precision mediump float;
        precision mediump int;
        varying vec2 y_texCoord;
        varying vec2 uv_texCoord;
        uniform sampler2D y_texture;
        uniform sampler2D u_texture;
        uniform sampler2D v_texture;
        uniform float alpha;
        uniform vec3 yuv_adj;
        uniform mat3 cc_matrix;
        void main()
        {
            float y_raw = texture2D(y_texture, y_texCoord).x;
            float u_unsigned = texture2D(u_texture, uv_texCoord).x;
            float v_unsigned = texture2D(v_texture, uv_texCoord).x;
            vec3 yuv = vec3(y_raw, u_unsigned, v_unsigned) + yuv_adj;
            vec3 rgb = cc_matrix * yuv;
            gl_FragColor = vec4(rgb, float(1)) * alpha;
        }
    );
}

FragmentShaderColor::FragmentShaderColor()
    : m_colorLocation(-1)
{
}

void FragmentShaderColor::init(GraphicsContext3D* context, unsigned program)
{
    m_colorLocation = context->getUniformLocation(program, "color");
    ASSERT(m_colorLocation != -1);
}

String FragmentShaderColor::getShaderString() const
{
    return SHADER(
        precision mediump float;
        uniform vec4 color;
        void main()
        {
            gl_FragColor = color;
        }
    );
}

FragmentShaderCheckerboard::FragmentShaderCheckerboard()
    : m_alphaLocation(-1)
    , m_texTransformLocation(-1)
    , m_frequencyLocation(-1)
{
}

void FragmentShaderCheckerboard::init(GraphicsContext3D* context, unsigned program)
{
    m_alphaLocation = context->getUniformLocation(program, "alpha");
    m_texTransformLocation = context->getUniformLocation(program, "texTransform");
    m_frequencyLocation = context->getUniformLocation(program, "frequency");
    ASSERT(m_alphaLocation != -1 && m_texTransformLocation != -1 && m_frequencyLocation != -1);
}

String FragmentShaderCheckerboard::getShaderString() const
{
    // Shader based on Example 13-17 of "OpenGL ES 2.0 Programming Guide"
    // by Munshi, Ginsburg, Shreiner.
    return SHADER(
        precision mediump float;
        precision mediump int;
        varying vec2 v_texCoord;
        uniform float alpha;
        uniform float frequency;
        uniform vec4 texTransform;
        void main()
        {
            vec4 color1 = vec4(1.0, 1.0, 1.0, 1.0);
            vec4 color2 = vec4(0.945, 0.945, 0.945, 1.0);
            vec2 texCoord = clamp(v_texCoord, 0.0, 1.0) * texTransform.zw + texTransform.xy;
            vec2 coord = mod(floor(texCoord * frequency * 2.0), 2.0);
            float picker = abs(coord.x - coord.y);
            gl_FragColor = mix(color1, color2, picker) * alpha;
        }
    );
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
