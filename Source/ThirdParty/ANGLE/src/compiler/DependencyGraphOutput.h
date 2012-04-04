//
//  DependencyGraph.h
//  ANGLE
//
//  Created by Max Vujovic on 3/27/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef COMPILER_DEPENDENCY_GRAPH_OUTPUT_H
#define COMPILER_DEPENDENCY_GRAPH_OUTPUT_H

#include "compiler/DependencyGraph.h"
#include "compiler/InfoSink.h"


// TDepGraphOutput

class TDepGraphOutput : public TDepGraphTraverser
{
public:
    TDepGraphOutput(TInfoSinkBase& sink) : mSink(sink) {}
    virtual void visitDepSymbol(TDepSymbol* depSymbol);
    virtual void visitDepArgument(TDepArgument* depParameter);
    virtual void visitFunctionCall(TDepFunctionCall* depFunctionCall);
    virtual void visitDepSelection(TDepSelection* depSelection);
    virtual void visitDepLoop(TDepLoop* depLoop);
    virtual void visitDepLogicalOp(TDepLogicalOp* depLogicalOp);
    
    void outputAllSpanningTrees(TDepGraph* depGraph);
private:
    void outputIndentation();

    TInfoSinkBase& mSink;
};

#endif
