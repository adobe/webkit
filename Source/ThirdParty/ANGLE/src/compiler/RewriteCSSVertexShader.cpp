//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/RewriteCSSShaderHelper.h"
#include "compiler/RewriteCSSVertexShader.h"

using namespace RewriteCSSShaderHelper;

const char* const RewriteCSSVertexShader::kTexCoordAttributeName = "a_texCoord";

void RewriteCSSVertexShader::rewrite()
{
    RewriteCSSShaderBase::rewrite();

    // TODO(mvujovic): We rely on the original shader to define a_texCoord.
    // In the future, all of the attributes and uniforms from the CSS Shaders spec will be built-ins,
    // and we will insert declarations for them here.
    insertTexCoordVaryingDeclaration();
    insertTexCoordVaryingAssignment();
}

// Inserts "varying vec2 css_v_texCoord;".
void RewriteCSSVertexShader::insertTexCoordVaryingDeclaration()
{
    TIntermSymbol* texCoordVarying = createSymbol(getTexCoordVaryingName(), vec2Type(EvqVaryingIn));
    TIntermAggregate* declaration = createDeclaration(texCoordVarying);
    insertAtBeginningOfShader(declaration);
}

// Inserts "css_v_texCoordXXX = a_texCoord;" as the first line of the main function.
void RewriteCSSVertexShader::insertTexCoordVaryingAssignment()
{
    TIntermSymbol* texCoordVarying = createSymbol(getTexCoordVaryingName(), vec2Type(EvqVaryingIn));
    TIntermSymbol* texCoordAttribute = createSymbol(kTexCoordAttributeName, vec2Type(EvqAttribute));
    TIntermBinary* assignment = createBinary(EOpAssign, texCoordVarying, texCoordAttribute, vec2Type(EvqTemporary));
    insertAtBeginningOfFunction(findFunction(kMain), assignment);
}
