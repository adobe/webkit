//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TODO: Need copyrights.
// TODO: Run valgrind on this thing and all of the tests.
// TODO: Audit destructors.
// TODO: Check that intermediate variables are created where appropriate.
// TODO: Investigate "dFdx, dFdy — return the partial derivative of an argument with respect to x or y", fwidth
// TODO: Investigate comma operator.
// TODO: Audit ptr vs reference usage for elegance.
// TODO: Audit type names for elegance.
// TODO: What is the node with id 0? Do we care?
// TODO: Should rename "node" to "intermNode" everywhere here? More specific types.
// TODO: Maybe generalize stl map lookups with find() further.
// TODO: Maybe consolidate some maps.
// TODO: Sprinkle consts in where they apply.
// TODO: Audit method ordering for elegance.
// TODO: Rename ValidateWebSafeFragmentShader to ValidateDecisionLimitations or something else.
// TODO: Make sure all the files / file names are appropriate.
// TODO: Verify in-code comments are good.
// TODO: Verify structural comments match project conventions.
// TODO: Use an output sink in DepGraphOutput, not printf.
// TODO: Clean up the vertex shader texture access checker.
// TODO: Create a flag for printing the dep graph.
// TODO: What to call the compiler flags?
// TODO: Validate texture access checks for all texture access. Maybe it should check for u_texture defintion instead.
// TODO: Look @ convention for private vs protected in the project. Audit the class defs.
// TODO: Switch case statements on one line or on mulitple lines? Double check this.

#ifndef COMPILER_VALIDATE_SAMPLER_LIMITATIONS_H_
#define COMPILER_VALIDATE_SAMPLER_LIMITATIONS_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/DependencyGraph.h"

using namespace std;

class TInfoSinkBase;

class ValidateWebSafeFragmentShader : TDepGraphTraverser {
public:
    ValidateWebSafeFragmentShader(TInfoSinkBase& sink);
    void validate(TDepGraph* depGraph);
    int numErrors() const { return mNumErrors; }
    
    virtual void visitDepArgument(TDepArgument* depParameter);
    virtual void visitFunctionCall(TDepFunctionCall* depFunctionCall);
    virtual void visitDepSymbol(TDepSymbol* depSymbol);
    virtual void visitDepSelection(TDepSelection* depSelection);
    virtual void visitDepLoop(TDepLoop* depLoop);
    virtual void visitDepLogicalOp(TDepLogicalOp* depLogicalOp);
    
private:
    void beginError(TIntermNode* node);
    void validateUserDefinedFunctionCallUsage(TDepGraph* depGraph);
    
	TInfoSinkBase& mSink;
    int mNumErrors;
};

#endif  // COMPILER_VALIDATE_COLOR_LIMITATIONS_H_
