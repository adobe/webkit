//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_VALIDATE_COLOR_LIMITATIONS_H_
#define COMPILER_VALIDATE_COLOR_LIMITATIONS_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"

class TInfoSinkBase;
typedef TVector<int> TUnsafeSymbols;

typedef TVector<TIntermLoop*> TMarkedLoops;
typedef TVector<TIntermSelection*> TMarkedSelections;
typedef TVector<TIntermAggregate*> TMarkedAggregate;
typedef TVector<TString> TDirtySymbols;

// Traverses intermediate tree to detect if unifrom textures are used.
class ValidateColorLimitations : public TIntermTraverser {
public:
    ValidateColorLimitations(TInfoSinkBase& sink);
    int numErrors() const { return mNumErrors; }
    bool foundNewUnsafeSymbols() const { return mNewUnsafeSymbols; }
    void setNewUnsafeSymbols(bool value) { mNewUnsafeSymbols = value; }

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit visit, TIntermBinary*);
    virtual bool visitUnary(Visit visit, TIntermUnary*);
    virtual bool visitSelection(Visit visit, TIntermSelection*);
    virtual bool visitAggregate(Visit visit, TIntermAggregate*);
    virtual bool visitLoop(Visit visit, TIntermLoop*);
    virtual bool visitBranch(Visit visit, TIntermBranch*);

private:
    void error(TSourceLoc loc, const char *reason, const char* token);
    bool isLoopMarked(const TIntermLoop*);
    bool isSelectionMarked(const TIntermSelection*);
    bool isAggregateMarked(const TIntermAggregate*);

	TInfoSinkBase& mSink;
    int mNumErrors;
    bool mNewUnsafeSymbols;
    TUnsafeSymbols mUnsafeSymbols;
    TMarkedLoops mMarkedLoops;
    TMarkedSelections mMarkedSelections;
    TMarkedAggregate mMarkedAggregate;
};

#endif  // COMPILER_VALIDATE_COLOR_LIMITATIONS_H_
