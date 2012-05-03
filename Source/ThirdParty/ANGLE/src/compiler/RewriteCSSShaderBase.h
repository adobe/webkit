//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_REWRITE_CSS_SHADER_BASE
#define COMPILER_REWRITE_CSS_SHADER_BASE

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/SymbolTable.h"

class TInfoSinkBase;

class RewriteCSSShaderBase {
public:
    RewriteCSSShaderBase(TIntermNode* root, const TSymbolTable& symbolTable, const TString& hiddenSymbolSuffix);

    virtual void rewrite();
    TIntermNode* getNewTreeRoot() { return mRoot; }

    virtual ~RewriteCSSShaderBase() {}

protected:
    static const char* const kMain;

    void insertAtBeginningOfShader(TIntermNode* node);
    void insertAtEndOfShader(TIntermNode* node);

    TIntermAggregate* findFunction(const TString& name) const;
    void renameFunction(const TString& oldFunctionName, const TString& newFunctionName);
    bool isSymbolUsed(const TString& symbolName);

    const TType& getBuiltinType(const TString& builtinName) const;
    const TString& getTexCoordVaryingName() const { return mTexCoordVaryingName; }

private:
    static const char* const kTexCoordVaryingPrefix;

    TIntermAggregate* mRoot;
    const TSymbolTable& mSymbolTable;
    TString mTexCoordVaryingName;

    TIntermAggregate* getRoot() const;

    void createRootSequenceIfNeeded();
};

#endif  // COMPILER_REWRITE_CSS_SHADER_BASE
