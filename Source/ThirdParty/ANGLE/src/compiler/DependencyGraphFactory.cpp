//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/DependencyGraphFactory.h"

TDepGraph* TDepGraphFactory::createDepGraph(TIntermNode* node)
{
    if (mDepGraph)
        delete mDepGraph;
    mDepGraph = new TDepGraph();

    mDepSetStack.clear();

    mIsUnderLeftSubTreeStack = TBoolStack();    // TOOD: Is there a beter way to clear an STL stack?
    mIsUnderLeftSubTreeStack.push(true);

    mLeftmostDepSymbolStack = TDepSymbolStack();
    mLeftmostDepSymbolStack.push(&NULL_DEP_SYMBOL);

    mIsGlobalScope = true;

    node->traverse(this);

    return mDepGraph;
}

TDepSymbol* TDepGraphFactory::getOrCreateDepSymbolByIntermSymbol(TIntermSymbol* node)
{
    TSymbolIdDepSymbolMap::iterator i = mDepGraph->getSymbolIdDepSymbolMap().find(node->getId());

    TDepSymbol* depSymbol = NULL;

    if (i != mDepGraph->getSymbolIdDepSymbolMap().end()) {
        TSymbolIdDepNodePair pair = *i;
        depSymbol = pair.second;
    } else {
        depSymbol = mDepGraph->createDepSymbol(node);
        TSymbolIdDepNodePair pair(node->getId(), depSymbol);
        mDepGraph->getSymbolIdDepSymbolMap().insert(pair);

        if (mIsGlobalScope) {
            // We map all symbols in the global scope by name, so traversers of the graph can quickly start searches at symbols like s_texture.
            TSymbolNameDepSymbolPair pair(node->getSymbol(), depSymbol);
            mDepGraph->getGlobalScopeSymbolNameDepSymbolMap().insert(pair);
        }
    }

    return depSymbol;
}

bool TDepGraphFactory::visitAggregate(Visit visit, TIntermAggregate* node)
{
    incrementDepth();

    TOperator op = node->getOp();
    switch (op) {
        case EOpFunction:
            visitFunctionDefinition(node);
            break;

        case EOpFunctionCall:
            visitFunctionCall(node);
            break;

        default:
            visitAggregateChildren(node);
            break;
    }

    decrementDepth();
    return false;
}

void TDepGraphFactory::visitFunctionDefinition(TIntermAggregate* node)
{
    // Currently, we do not support user defined functions.
    if (node->getName() != "main(")
        return;

    mIsGlobalScope = false;

    visitAggregateChildren(node);

    mIsGlobalScope = true;
}

void TDepGraphFactory::visitFunctionCall(TIntermAggregate* node)
{
    TDepFunctionCall* depFunctionCall = mDepGraph->createDepFunctionCall(node);

    // Run through the function call arguments.
    int argumentNumber = 0;
    TIntermSequence& arguments = node->getSequence();
    for(TIntermSequence::iterator i = arguments.begin(); i != arguments.end(); i++, argumentNumber++)
    {
        mDepSetStack.pushSet();

        TIntermNode* argument = *i;
        argument->traverse(this);
        TDepParentSet& argumentDepSet = mDepSetStack.topSet();

        // y = f(x) => x->depArgument(0)->depFunctionCall->y
        if (!argumentDepSet.empty()) {
            TDepArgument* depArgument = mDepGraph->createDepArgument(node, argumentNumber);
            connectDepSetToDepNode(argumentDepSet, depArgument);
            depArgument->addDepNode(depFunctionCall);
        }

        mDepSetStack.popSet();
    }

    mDepSetStack.insertIntoTopSet(depFunctionCall);
}

void TDepGraphFactory::visitAggregateChildren(TIntermAggregate* node)
{
    TIntermSequence& sequence = node->getSequence();
    for(TIntermSequence::iterator i = sequence.begin(); i != sequence.end(); i++)
    {
        TIntermNode* intermNode = *i;
        intermNode->traverse(this);
    }
}

void TDepGraphFactory::visitSymbol(TIntermSymbol* node)
{
    TDepSymbol* depSymbol = getOrCreateDepSymbolByIntermSymbol(node);
    mDepSetStack.insertIntoTopSet(depSymbol);

    // If this is the leftmost symbol under an assignment, set it.
    if (mIsUnderLeftSubTreeStack.top()) {
        mLeftmostDepSymbolStack.pop();
        mLeftmostDepSymbolStack.push(depSymbol);
    }
}

void TDepGraphFactory::visitConstantUnion(TIntermConstantUnion*)
{
}

bool TDepGraphFactory::visitBinary(Visit visit, TIntermBinary* node)
{
    incrementDepth();

    TOperator op = node->getOp();
    if (op == EOpInitialize || node->modifiesState())
        visitAssignment(node);
    else if (op == EOpLogicalAnd || op == EOpLogicalOr)
        visitLogicalOp(node);
    else
        visitBinaryChildren(node);

    decrementDepth();
    return false;
}

void TDepGraphFactory::visitAssignment(TIntermBinary* node)
{
    TDepSymbol* leftmostDepSymbol = NULL;

    if (node->getLeft()) {
        mDepSetStack.pushSet();
        mLeftmostDepSymbolStack.push(&NULL_DEP_SYMBOL);
        mIsUnderLeftSubTreeStack.push(true);

        node->getLeft()->traverse(this);
        TDepParentSet& leftDepSet = mDepSetStack.topSet();
        leftmostDepSymbol = mLeftmostDepSymbolStack.top();
        connectDepSetToDepNode(leftDepSet, leftmostDepSymbol);

        mIsUnderLeftSubTreeStack.pop();
        mLeftmostDepSymbolStack.pop();
        mDepSetStack.popSet();
    }

    if (node->getRight()) {
        mDepSetStack.pushSet();
        mIsUnderLeftSubTreeStack.push(false);

        node->getRight()->traverse(this);
        TDepParentSet& rightDepSet = mDepSetStack.topSet();
        connectDepSetToDepNode(rightDepSet, leftmostDepSymbol);

        mIsUnderLeftSubTreeStack.pop();
        mDepSetStack.popSet();
    }

    // a = (b = c) ==> a->b->c
    mDepSetStack.insertIntoTopSet(leftmostDepSymbol);
}

void TDepGraphFactory::visitLogicalOp(TIntermBinary* node)
{
    if (node->getLeft()) {
        mDepSetStack.pushSet();
        mIsUnderLeftSubTreeStack.push(mIsUnderLeftSubTreeStack.top());

        node->getLeft()->traverse(this);
        TDepParentSet leftDepSet = mDepSetStack.topSet();
        if (!leftDepSet.empty()) {
            TDepLogicalOp* depLogicalOp = mDepGraph->createDepLogicalOp(node);
            connectDepSetToDepNode(leftDepSet, depLogicalOp);
        }

        mIsUnderLeftSubTreeStack.pop();
        mDepSetStack.combineTopTwoSets();
    }

    if (node->getRight()) {
        mIsUnderLeftSubTreeStack.push(false);

        node->getRight()->traverse(this);

        mIsUnderLeftSubTreeStack.pop();
    }
}

void TDepGraphFactory::visitBinaryChildren(TIntermBinary* node)
{
    if (node->getLeft()) {
        mIsUnderLeftSubTreeStack.push(mIsUnderLeftSubTreeStack.top());

        node->getLeft()->traverse(this);

        mIsUnderLeftSubTreeStack.pop();
    }

    if (node->getRight()) {
        mIsUnderLeftSubTreeStack.push(false);

        node->getRight()->traverse(this);

        mIsUnderLeftSubTreeStack.pop();
    }
}

void TDepGraphFactory::connectDepSetToDepNode(TDepParentSet& depNodeSet, TDepNode* depNode)
{
    for (TDepParentSet::iterator i = depNodeSet.begin(); i != depNodeSet.end(); i++)
    {
        TDepParent* currentDepNode = *i;
        currentDepNode->addDepNode(depNode);
    }
}

bool TDepGraphFactory::visitUnary(Visit visit, TIntermUnary* node)
{
    return true;
}

bool TDepGraphFactory::visitSelection(Visit visit, TIntermSelection* node)
{
    incrementDepth();

    mDepSetStack.pushSet();

    node->getCondition()->traverse(this);
    TDepParentSet& conditionDepSet = mDepSetStack.topSet();
    if (!conditionDepSet.empty()) {
        TDepSelection* depSelection = mDepGraph->createDepSelection(node);
        connectDepSetToDepNode(conditionDepSet, depSelection);
    }

    mDepSetStack.popSet();

    if (node->getTrueBlock())
        node->getTrueBlock()->traverse(this);

    if (node->getFalseBlock())
        node->getFalseBlock()->traverse(this);

    decrementDepth();
    return false;
}

bool TDepGraphFactory::visitLoop(Visit visit, TIntermLoop* node)
{
    incrementDepth();

    if (node->getCondition()) {
        mDepSetStack.pushSet();

        node->getCondition()->traverse(this);
        TDepParentSet& conditionDepSet = mDepSetStack.topSet();
        if (!conditionDepSet.empty()) {
            TDepLoop* depLoop = mDepGraph->createDepLoop(node);
            connectDepSetToDepNode(conditionDepSet, depLoop);
        }

        mDepSetStack.popSet();
    }

    if (node->getBody())
        node->getBody()->traverse(this);

    if (node->getExpression())
        node->getExpression()->traverse(this);

    decrementDepth();
    return false;
}

bool TDepGraphFactory::visitBranch(Visit visit, TIntermBranch* node)
{
    return true;
}
