//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/ValidateColorLimitations.h"
#include "compiler/InfoSink.h"
#include "compiler/ParseHelper.h"

namespace {
bool isSymbolSafe(const TUnsafeSymbols& unsafeSymbols, const TIntermSymbol *symbol) {
for (TUnsafeSymbols::const_iterator i = unsafeSymbols.begin(); i != unsafeSymbols.end(); ++i) {
        if (*i == symbol->getId())
            return false;
    }
    return true;
}

class ValidateColorUsage : public TIntermTraverser {
public:
    ValidateColorUsage(TInfoSinkBase& sink, TUnsafeSymbols& unsafeSymbols, TDirtySymbols &dirtySymbols)
        : mDirtyNode(false)
        , mSink(sink)
        , mUnsafeSymbols(unsafeSymbols)
        , mDirtySymbols(dirtySymbols)
    {
    }

    // Returns true if the parsed node represents a constant index expression.
    bool usesDirtyNode() const { return mDirtyNode; }
    
    

    virtual void visitSymbol(TIntermSymbol* symbol) {
        bool isUnsafe = symbol->getDirtyFlag() || !isSymbolSafe(mUnsafeSymbols, symbol);
        mDirtyNode = mDirtyNode || isUnsafe;
        if (mDirtyNode)
            mDirtySymbols.push_back(symbol->getSymbol());
    }

    virtual bool visitAggregate(Visit visit, TIntermAggregate* node) {
        if (node->getOp() == EOpFunctionCall) {
            TString name = TFunction::unmangleName(node->getName());
            if (name == "texture2D") {
                mDirtyNode = true;
                mDirtySymbols.push_back(name);
            }
        }
        return true;
    }

private:
    bool mDirtyNode;
    const TUnsafeSymbols& mUnsafeSymbols;
    TDirtySymbols& mDirtySymbols;
    TInfoSinkBase& mSink;
};

}

ValidateColorLimitations::ValidateColorLimitations(TInfoSinkBase& sink)
    : TIntermTraverser(true, false, true)
    , mSink(sink) 
    , mNumErrors(0)
    , mNewUnsafeSymbols(false)
{
}

void ValidateColorLimitations::error(TSourceLoc loc,
                                const char *reason, const char* token)
{
    mSink.prefix(EPrefixError);
    mSink.location(loc);
    mSink << "'" << token << "' : " << reason << "\n";
    ++mNumErrors;
}

bool ValidateColorLimitations::isLoopMarked(const TIntermLoop *loop) {
for (TMarkedLoops::const_iterator i = mMarkedLoops.begin(); i != mMarkedLoops.end(); ++i) {
        if (*i == loop)
            return true;
    }
    return false;
}

bool ValidateColorLimitations::isSelectionMarked(const TIntermSelection *selection) {
for (TMarkedSelections::const_iterator i = mMarkedSelections.begin(); i != mMarkedSelections.end(); ++i) {
        if (*i == selection)
            return true;
    }
    return false;
}

bool ValidateColorLimitations::isAggregateMarked(const TIntermAggregate *node) {
    for (TMarkedAggregate::const_iterator i = mMarkedAggregate.begin(); i != mMarkedAggregate.end(); ++i) {
        if (*i == node)
            return true;
    }
    return false;
}

bool ValidateColorLimitations::visitAggregate(Visit visit, TIntermAggregate* node)
{
    if (isAggregateMarked(node))
        return false;
	switch (node->getOp()) {
        case EOpFunctionCall: {
            TString name = TFunction::unmangleName(node->getName());
            if (name == "texture2D") {
                node->setDirtyFlag(true);
                //mSink.location(node->getLine());
                //mSink << "Dirty: texture2D\n";
            }
            break;
        }
        case EOpPow: {
            // Traverse function parameters.
            TDirtySymbols dirtySymbols;
            ValidateColorUsage validate(mSink, mUnsafeSymbols, dirtySymbols);
            node->traverse(&validate);
            if (validate.usesDirtyNode()) {
                mSink.location(node->getLine());
                mSink << "ERROR: ";
                mSink << "Pow ";
                mSink << "[ ";
                for (TDirtySymbols::const_iterator i = dirtySymbols.begin(); i != dirtySymbols.end(); ++i)
                {
                    mSink << *i << " ";
                }
                mSink << "] ";
                mSink << " Not allowed to make color-based decisions" << '\n';
                mNumErrors ++;
                node->setDirtyFlag(true);
                mMarkedAggregate.push_back(node);
            }
            break;
        }
      default:
        break;
    }
    return true;
}

void ValidateColorLimitations::visitSymbol(TIntermSymbol* node) 
{
}

void ValidateColorLimitations::visitConstantUnion(TIntermConstantUnion*) 
{
}

bool ValidateColorLimitations::visitBinary(Visit visit, TIntermBinary* node) 
{
    if (node->modifiesState()) {
         TIntermSymbol* symbol = node->getLeft()->getAsSymbolNode();
         //if (symbol) 
         //    mSink << "RHU(check): " << symbol->getSymbol() << (symbol->getDirtyFlag() ? " (dirty)\n" : "\n");
         if (symbol && !symbol->getDirtyFlag() && node->getRight() != NULL) {
            TDirtySymbols dirtySymbols;
            ValidateColorUsage validate(mSink, mUnsafeSymbols, dirtySymbols);
            node->getRight()->traverse(&validate);
            bool dirty = validate.usesDirtyNode();
            if (dirty) {
                if (isSymbolSafe(mUnsafeSymbols, symbol)) {
                    mUnsafeSymbols.push_back(symbol->getId());
                    mNewUnsafeSymbols = true;
                    mSink.location(symbol->getLine());
                    mSink << "Dirty: " << symbol->getSymbol() << '(' << symbol->getId() << ")\n";
                }
                symbol->setDirtyFlag(dirty);
            }
         }

    }
    switch (node->getOp()) {
        case EOpInitialize: {
            bool dirty = false;
            if (node->getRight() != NULL) {
                TDirtySymbols dirtySymbols;
                ValidateColorUsage validate(mSink, mUnsafeSymbols, dirtySymbols);
                node->getRight()->traverse(&validate);
                dirty = validate.usesDirtyNode();
                //mSink << (dirty ? " found dirty " : "") << '\n';
            }
            if (node->getLeft() != NULL && node->getLeft()->getAsSymbolNode()) {
                TIntermSymbol* symbol = node->getLeft()->getAsSymbolNode();
                if (dirty) {
                    if (isSymbolSafe(mUnsafeSymbols, symbol)) {
                        mUnsafeSymbols.push_back(symbol->getId());
                        mNewUnsafeSymbols = true;
                        mSink.location(symbol->getLine());
                        mSink << "Dirty(init): " << symbol->getSymbol() << '(' << symbol->getId() << ")\n";
                    }
                   symbol->setDirtyFlag(dirty);
                }
                
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool ValidateColorLimitations::visitUnary(Visit visit, TIntermUnary*) 
{
    return true;
}

bool ValidateColorLimitations::visitSelection(Visit visit, TIntermSelection* node) 
{
    if (isSelectionMarked(node))
        return false;
    TDirtySymbols dirtySymbols;
    ValidateColorUsage validate(mSink, mUnsafeSymbols, dirtySymbols);
    bool dirty = validate.usesDirtyNode();
    node->getCondition()->traverse(&validate);
    if (validate.usesDirtyNode()) {
        mSink.location(node->getLine());
        mSink << "ERROR: ";
        mSink << "If ";
        mSink << "[ ";
        for (TDirtySymbols::const_iterator i = dirtySymbols.begin(); i != dirtySymbols.end(); ++i)
        {
            mSink << *i << " ";
        }
        mSink << "] ";
        mSink << " Not allowed to make color-based decisions" << '\n';
        mNumErrors ++;
        mMarkedSelections.push_back(node);
    }
    return true;
}

bool ValidateColorLimitations::visitLoop(Visit visit, TIntermLoop* node) 
{ 
    if (isLoopMarked(node))
        return false;
    if (node->getCondition()) {
        TDirtySymbols dirtySymbols;
        ValidateColorUsage validate(mSink, mUnsafeSymbols, dirtySymbols);
        bool dirty = validate.usesDirtyNode();
        dirtySymbols.clear();
        node->getCondition()->traverse(&validate);
        if (validate.usesDirtyNode()) {
            mSink.location(node->getLine());
            mSink << "ERROR: ";
            if (node->getType() == ELoopFor)
                mSink << "For ";
            else if (node->getType() == ELoopWhile)
                mSink << "While ";
            else if (node->getType() == ELoopDoWhile)
                mSink  << "DoWhile ";
            mSink << "[ ";
            for (TDirtySymbols::const_iterator i = dirtySymbols.begin(); i != dirtySymbols.end(); ++i)
            {
                mSink << *i << " ";
            }
            mSink << "] ";
            mSink << "Not allowed to make color-based decisions" << '\n';
            mNumErrors ++;
            mMarkedLoops.push_back(node);
        }
    }
    return true;
}

bool ValidateColorLimitations::visitBranch(Visit visit, TIntermBranch*) 
{
    return true;
}