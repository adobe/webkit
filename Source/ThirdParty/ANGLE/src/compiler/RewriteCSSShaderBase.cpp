//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/FindSymbolUsage.h"
#include "compiler/RenameFunction.h"
#include "compiler/RewriteCSSShaderBase.h"
#include "compiler/RewriteCSSShaderHelper.h"

using namespace RewriteCSSShaderHelper;

RewriteCSSShaderBase::RewriteCSSShaderBase(TIntermNode* root, const TSymbolTable& symbolTable, const TString& hiddenSymbolSuffix)
    : mRoot(root->getAsAggregate())
    , mSymbolTable(symbolTable)
    , mTexCoordVaryingName(kTexCoordVaryingPrefix + hiddenSymbolSuffix)
{
    // The intermediate tree root should come in as an aggregate node.
    // It should be either a sequence or the main function declaration.
    // Previous compiler steps should have already thrown an error and exited if there is no main function.
    ASSERT(mRoot);
    ASSERT(mRoot->getOp() == EOpSequence || (mRoot->getOp() == EOpFunction && mRoot->getName() == kMain));
}

void RewriteCSSShaderBase::rewrite()
{
    createRootSequenceIfNeeded();
}

const char* const RewriteCSSShaderBase::kMain = "main(";
const char* const RewriteCSSShaderBase::kTexCoordVaryingPrefix = "css_v_texCoord";

void RewriteCSSShaderBase::insertAtBeginningOfShader(TIntermNode* node)
{
    TIntermSequence& rootSequence = getRoot()->getSequence();
    rootSequence.insert(rootSequence.begin(), node);
}

void RewriteCSSShaderBase::insertAtEndOfShader(TIntermNode* node)
{
    getRoot()->getSequence().push_back(node);
}

TIntermAggregate* RewriteCSSShaderBase::findFunction(const TString& name) const
{
    TIntermSequence& rootSequence = getRoot()->getSequence();
    for (TIntermSequence::const_iterator iter = rootSequence.begin(); iter != rootSequence.end(); ++iter) {
        TIntermNode* node = *iter;
        TIntermAggregate* aggregate = node->getAsAggregate();
        if (aggregate && aggregate->getOp() == EOpFunction && aggregate->getName() == name)
            return aggregate;
    }
    return NULL;
}

void RewriteCSSShaderBase::renameFunction(const TString& oldFunctionName, const TString& newFunctionName)
{
    RenameFunction renameFunction(oldFunctionName, newFunctionName);
    getRoot()->traverse(&renameFunction);
}


bool RewriteCSSShaderBase::isSymbolUsed(const TString& symbolName)
{
    FindSymbolUsage findSymbolUsage(symbolName);
    getRoot()->traverse(&findSymbolUsage);
    return findSymbolUsage.symbolUsageFound();
}

const TType& RewriteCSSShaderBase::getBuiltinType(const TString& builtinName) const
{
    TSymbol* builtinSymbol = mSymbolTable.find(builtinName);
    ASSERT(builtinSymbol->isVariable());
    TVariable* builtinVariable = static_cast<TVariable*>(builtinSymbol);
    return builtinVariable->getType();
}

TIntermAggregate* RewriteCSSShaderBase::getRoot() const
{
    // createRootSequenceIfNeeded should have been called at the beginning of the rewrite,
    // so the tree should be a sequence.
    ASSERT(mRoot->getOp() == EOpSequence);
    return mRoot;
}

// The tree root comes in as either a sequence or a main function declaration.
// If the tree root is a main function declaration, this method creates a new sequence,
// puts the main function declaration inside it, and changes the tree root to point to
// the new sequence.
// Thus, after this function is called, the tree root can only be a sequence.
void RewriteCSSShaderBase::createRootSequenceIfNeeded()
{
    if (mRoot->getOp() == EOpFunction) {
        TIntermAggregate* newRoot = new TIntermAggregate(EOpSequence);
        TIntermSequence& sequence = newRoot->getSequence();
        sequence.push_back(mRoot);
        mRoot = newRoot;
    }
}
