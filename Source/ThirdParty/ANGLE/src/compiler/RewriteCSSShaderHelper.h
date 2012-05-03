//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_REWRITE_CSS_SHADER_HELPER
#define COMPILER_REWRITE_CSS_SHADER_HELPER

#include "compiler/intermediate.h"

namespace RewriteCSSShaderHelper {
    const TType voidType(TQualifier qualifier);
    const TType vec2Type(TQualifier qualifier);
    const TType vec4Type(TQualifier qualifier);
    const TType mat4Type(TQualifier qualifier);
    const TType sampler2DType();
    const TPrecision getDefaultPrecision(TQualifier qualifier);

    TIntermConstantUnion* createVec4Constant(float x, float y, float z, float w);
    TIntermConstantUnion* createMat4IdentityConstant();
    TIntermSymbol* createSymbol(const TString& name, const TType& type);
    TIntermAggregate* createDeclaration(TIntermNode* child);
    TIntermAggregate* createDeclaration(TIntermSymbol* symbol, TIntermTyped* rhs);
    TIntermBinary* createBinary(TOperator op, TIntermTyped* left, TIntermTyped* right, const TType& type);
    TIntermAggregate* createFunction(const TString& name, const TType& returnType);
    TIntermAggregate* createFunctionCall(const TString& name, const TType& returnType);
    TIntermAggregate* createFunctionCall(const TString& name, TIntermNode* argument1, TIntermNode* argument2, const TType& returnType);
    void addArgument(TIntermAggregate* functionCall, TIntermNode* argument);

    void insertAtBeginningOfFunction(TIntermAggregate* function, TIntermNode* node);
    void insertAtEndOfFunction(TIntermAggregate* function, TIntermNode* node);
}

#endif
