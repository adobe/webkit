//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_REWRITE_CSS_VERTEX_SHADER
#define COMPILER_REWRITE_CSS_VERTEX_SHADER

#include "GLSLANG/ShaderLang.h"

#include "compiler/RewriteCSSShaderBase.h"

//
// Rewrites a CSS vertex shader's intermediate tree into a valid GLSL shader.
//

// Example original vertex shader:
/*

 attribute vec2 a_texCoord;
 void main() {
 }

 */

// Example rewritten vertex shader:
// "XXX" represents the hidden symbol suffix.
/*

 varying vec2 css_v_texCoordXXX;
 attribute vec2 a_texCoord;
 void main(){
 (css_v_texCoordXXX = a_texCoord);
 }

 */

class RewriteCSSVertexShader : public RewriteCSSShaderBase {
public:
    RewriteCSSVertexShader(TIntermNode* root, const TSymbolTable& symbolTable, const TString& hiddenSymbolSuffix)
        : RewriteCSSShaderBase(root, symbolTable, hiddenSymbolSuffix) {}

    void rewrite();

private:
    static const char* const kTexCoordAttributeName;

    void insertTexCoordVaryingDeclaration();
    void insertTexCoordVaryingAssignment();
};

#endif  // COMPILER_REWRITE_CSS_VERTEX_SHADER
