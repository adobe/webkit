//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/DependencyGraphOutput.h"
#include "compiler/InfoSink.h"
#include "compiler/ParseHelper.h"
#include "compiler/ValidateWebSafeFragmentShader.h"

ValidateWebSafeFragmentShader::ValidateWebSafeFragmentShader(TInfoSinkBase& sink)
    : mSink(sink) 
    , mNumErrors(0)
{
}

void ValidateWebSafeFragmentShader::validate(TDepGraph* depGraph)
{
    mNumErrors = 0;
    
    validateUserDefinedFunctionCallUsage(depGraph);
    
    // Traverse the dependency graph starting at u_texture and generate an error each time we hit a condition nodes.
    TSymbolNameDepSymbolMap::iterator i = depGraph->getGlobalScopeSymbolNameDepSymbolMap().find((TString)"u_texture");
    if (i != depGraph->getGlobalScopeSymbolNameDepSymbolMap().end()) {
        TSymbolNameDepSymbolPair pair = *i;
        TDepSymbol* uTextureDepSymbol = pair.second;
        uTextureDepSymbol->traverse(this);
    }
}

// The dependency graph does not support user defined function calls, so generate errors for them.
void ValidateWebSafeFragmentShader::validateUserDefinedFunctionCallUsage(TDepGraph* depGraph)
{
    TDepFunctionCallVector userDefinedDepFunctionCalls = depGraph->getUserDefinedDepFunctionCalls();
    for (TDepFunctionCallVector::iterator i = userDefinedDepFunctionCalls.begin(); i != userDefinedDepFunctionCalls.end(); i++)
    {
        TDepFunctionCall* depFunctionCall = *i;
        beginError(depFunctionCall->getIntermFunctionCall());
        mSink << "Calls to user defined functions are not permitted.\n";
    }    
}

void ValidateWebSafeFragmentShader::beginError(TIntermNode* node)
{
    mNumErrors++;
    mSink.prefix(EPrefixError);
    mSink.location(node->getLine());
}

void ValidateWebSafeFragmentShader::visitDepArgument(TDepArgument* depParameter)
{
    if (!(depParameter->getIntermFunctionCall()->getName() == "texture2D(s21;vf2;" && depParameter->getArgumentNumber() == 1))
        return;
    
    beginError(depParameter->getIntermFunctionCall());    
    mSink << "Values derived from symbol 'u_texture' are not permitted to be the second argument of a texture2D call.\n";  
}

void ValidateWebSafeFragmentShader::visitFunctionCall(TDepFunctionCall* depFunctionCall)
{
}

void ValidateWebSafeFragmentShader::visitDepSymbol(TDepSymbol* depSymbol)
{
}

void ValidateWebSafeFragmentShader::visitDepSelection(TDepSelection* depSelection)
{
    beginError(depSelection->getIntermSelection());
    mSink << "Values derived from symbol 'u_texture' are not permitted in conditional statements.\n";
}

void ValidateWebSafeFragmentShader::visitDepLoop(TDepLoop* depLoop)
{
    beginError(depLoop->getIntermLoop());
    mSink << "Values derived from symbol 'u_texture' are not permitted in loop conditions.\n";
}

void ValidateWebSafeFragmentShader::visitDepLogicalOp(TDepLogicalOp* depLogicalOp)
{
    beginError(depLogicalOp->getIntermLogicalOp());
    mSink << "Values derived from symbol 'u_texture' are not permitted on the left hand side of a logical " << depLogicalOp->getOpString() << " operator.\n";
}
