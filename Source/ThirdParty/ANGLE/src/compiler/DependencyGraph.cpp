//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/DependencyGraph.h"

TDepGraph::~TDepGraph()
{
    for (TDepNodeVector::iterator i = mAllDepNodes.begin(); i != mAllDepNodes.end(); i++)
    {
        TDepNode* depNode = *i;
        delete depNode;
    }
}

TDepArgument* TDepGraph::createDepArgument(TIntermAggregate* intermFunctionCall, int argumentNumber)
{
    TDepArgument* depArgument = new TDepArgument(intermFunctionCall, argumentNumber);
    mAllDepNodes.push_back(depArgument);
    return depArgument;
}

TDepFunctionCall* TDepGraph::createDepFunctionCall(TIntermAggregate* intermFunctionCall)
{
    TDepFunctionCall* depFunctionCall = new TDepFunctionCall(intermFunctionCall);
    mAllDepNodes.push_back(depFunctionCall);
    if (depFunctionCall->getIntermFunctionCall()->isUserDefined())
        mUserDefinedDepFunctionCalls.push_back(depFunctionCall);
    return depFunctionCall;    
}

TDepSymbol* TDepGraph::createDepSymbol(TIntermSymbol* intermSymbol)
{
    TDepSymbol* depSymbol = new TDepSymbol(intermSymbol);
    mAllDepNodes.push_back(depSymbol);
    return depSymbol;
}

TDepSelection* TDepGraph::createDepSelection(TIntermSelection* intermSelection)
{
    TDepSelection* depSelection = new TDepSelection(intermSelection);
    mAllDepNodes.push_back(depSelection);
    return depSelection;
}

TDepLoop* TDepGraph::createDepLoop(TIntermLoop* intermLoop)
{
    TDepLoop* depLoop = new TDepLoop(intermLoop);
    mAllDepNodes.push_back(depLoop);
    return depLoop;
}

TDepLogicalOp* TDepGraph::createDepLogicalOp(TIntermBinary* intermLogicalOp)
{
    TDepLogicalOp* depLogicalOp = new TDepLogicalOp(intermLogicalOp);
    mAllDepNodes.push_back(depLogicalOp);
    return depLogicalOp;
}

const char* TDepLogicalOp::getOpString()
{
    const char* opString = NULL;
    switch (mIntermLogicalOp->getOp()) {
        case EOpLogicalAnd: opString = "and"; break;
        case EOpLogicalOr: opString = "or"; break;
        default: opString = "unknown"; break;
    }
    return opString;
}
