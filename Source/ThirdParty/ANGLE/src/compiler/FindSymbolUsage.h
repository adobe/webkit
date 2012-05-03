//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_FIND_SYMBOL_USAGE
#define COMPILER_FIND_SYMBOL_USAGE

#include "compiler/intermediate.h"

//
// Determines if a symbol is used.
//
class FindSymbolUsage : public TIntermTraverser
{
public:
    FindSymbolUsage(const TString& symbolName)
    : TIntermTraverser(true, false, false)
    , mSymbolName(symbolName)
    , mSymbolUsageFound(false) {}

    bool symbolUsageFound() { return mSymbolUsageFound; }

    virtual void visitSymbol(TIntermSymbol* node)
    {
        if (node->getSymbol() == mSymbolName)
            mSymbolUsageFound = true;
    }

    virtual bool visitBinary(Visit visit, TIntermBinary*) { return shouldKeepLooking(); }
    virtual bool visitUnary(Visit visit, TIntermUnary*) { return shouldKeepLooking(); }
    virtual bool visitSelection(Visit visit, TIntermSelection*) { return shouldKeepLooking(); }
    virtual bool visitAggregate(Visit visit, TIntermAggregate*) { return shouldKeepLooking(); }
    virtual bool visitLoop(Visit visit, TIntermLoop*) { return shouldKeepLooking(); }
    virtual bool visitBranch(Visit visit, TIntermBranch*) { return shouldKeepLooking(); }

private:
    bool shouldKeepLooking() { return !mSymbolUsageFound; }

    const TString& mSymbolName;
    bool mSymbolUsageFound;
};


#endif
