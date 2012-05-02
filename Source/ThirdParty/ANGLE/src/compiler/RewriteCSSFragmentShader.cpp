//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/RewriteCSSFragmentShader.h"
#include "compiler/RewriteCSSShaderHelper.h"

using namespace RewriteCSSShaderHelper;

void RewriteCSSFragmentShader::rewrite()
{
    RewriteCSSShaderBase::rewrite();

    bool usesBlendColor = isSymbolUsed(kBlendColor);
    bool usesColorMatrix = isSymbolUsed(kColorMatrix);

    insertTextureUniformDeclaration();
    insertTexCoordVaryingDeclaration();
    if (usesBlendColor)
        insertBlendColorDeclaration();
    if (usesColorMatrix)
        insertColorMatrixDeclaration();

    renameFunction(kMain, mUserMainFunctionName);

    TIntermAggregate* newMainFunction = insertNewMainFunction();
    insertUserMainFunctionCall(newMainFunction);
    insertBlendOp(newMainFunction, usesBlendColor, usesColorMatrix);
}

const char* const RewriteCSSFragmentShader::kBlendColor = "css_BlendColor";
const char* const RewriteCSSFragmentShader::kColorMatrix = "css_ColorMatrix";
const char* const RewriteCSSFragmentShader::kTextureUniformPrefix = "css_u_texture";
const char* const RewriteCSSFragmentShader::kUserMainFunctionPrefix = "css_main";
const char* const RewriteCSSFragmentShader::kFragColor = "gl_FragColor";
const char* const RewriteCSSFragmentShader::kTexture2D = "texture2D(s21;vf2;";

// Inserts "vec4 css_BlendColor = vec4(1.0, 1.0, 1.0, 1.0);".
void RewriteCSSFragmentShader::insertBlendColorDeclaration()
{
    TIntermSymbol* blendColor = createSymbol(kBlendColor, getBuiltinType(kBlendColor));
    TIntermConstantUnion* constant = createVec4Constant(1.0f, 1.0f, 1.0f, 1.0f);
    TIntermAggregate* declaration = createDeclaration(blendColor, constant);
    insertAtBeginningOfShader(declaration);
}

// Inserts "mat4 css_ColorMatrix = mat4(1.0, 0.0, 0.0, 0.0 ...);".
void RewriteCSSFragmentShader::insertColorMatrixDeclaration()
{
    TIntermSymbol* colorMatrix = createSymbol(kColorMatrix, getBuiltinType(kColorMatrix));
    TIntermConstantUnion* identityMatrix = createMat4IdentityConstant();
    TIntermAggregate* declaration = createDeclaration(colorMatrix, identityMatrix);
    insertAtBeginningOfShader(declaration);
}

// Inserts "uniform sampler2D css_u_texture_XXX;".
void RewriteCSSFragmentShader::insertTextureUniformDeclaration()
{
    TIntermSymbol* textureUniform = createSymbol(mTextureUniformName, sampler2DType());
    TIntermAggregate* declaration = createDeclaration(textureUniform);
    insertAtBeginningOfShader(declaration);
}

// Inserts "varying vec2 css_v_texCoord;".
void RewriteCSSFragmentShader::insertTexCoordVaryingDeclaration()
{
    TIntermSymbol* texCoordVarying = createSymbol(getTexCoordVaryingName(), vec2Type(EvqVaryingIn));
    TIntermAggregate* declaration = createDeclaration(texCoordVarying);
    insertAtBeginningOfShader(declaration);
}

// Inserts "void main() {}" and returns the new main function.
TIntermAggregate* RewriteCSSFragmentShader::insertNewMainFunction()
{
    TIntermAggregate* newMainFunction = createFunction(kMain, voidType(EvqGlobal));
    insertAtEndOfShader(newMainFunction);
    return newMainFunction;
}

// Inserts "css_mainXXX();" at the beginning of the passed-in function.
void RewriteCSSFragmentShader::insertUserMainFunctionCall(TIntermAggregate* function)
{
    TIntermAggregate* userMainFunctionCall = createFunctionCall(mUserMainFunctionName, voidType(EvqTemporary));
    insertAtBeginningOfFunction(function, userMainFunctionCall);
}

// Inserts "gl_FragColor = (css_ColorMatrix * texture2D(css_u_textureXXX, css_v_texCoordXXX)) <BLEND OP> css_FragColor;"
// at the beginning of the passed-in function if both css_BlendColor and css_ColorMatrix are used.
// If they are not used in the shader, they are used in the blend op expression.
void RewriteCSSFragmentShader::insertBlendOp(TIntermAggregate* function, bool usesBlendColor, bool usesColorMatrix)
{
    // TODO(mvujovic): In the future, we'll support more blend operations than just multiply.
    const TOperator blendOp = EOpMul;

    TIntermSymbol* textureUniform = createSymbol(mTextureUniformName, sampler2DType());
    TIntermSymbol* texCoordVarying = createSymbol(getTexCoordVaryingName(), vec2Type(EvqVaryingIn));
    TIntermAggregate* texture2DCall = createFunctionCall(kTexture2D, textureUniform, texCoordVarying, vec4Type(EvqTemporary));

    TIntermTyped* blendOpLhs = texture2DCall;
    if (usesColorMatrix) {
        TIntermSymbol* colorMatrix = createSymbol(kColorMatrix, mat4Type(EvqGlobal));
        blendOpLhs = createBinary(EOpMatrixTimesVector, colorMatrix, texture2DCall, vec4Type(EvqTemporary));
    }

    TIntermTyped* assignmentRhs = blendOpLhs;
    if (usesBlendColor) {
        TIntermSymbol* blendColor = createSymbol(kBlendColor, vec4Type(EvqGlobal));
        assignmentRhs = createBinary(blendOp, blendOpLhs, blendColor, vec4Type(EvqTemporary));
    }

    TIntermSymbol* fragColor = createSymbol(kFragColor, vec4Type(EvqFragColor));
    TIntermBinary* assignment = createBinary(EOpAssign, fragColor, assignmentRhs, vec4Type(EvqTemporary));
    insertAtEndOfFunction(function, assignment);
}
