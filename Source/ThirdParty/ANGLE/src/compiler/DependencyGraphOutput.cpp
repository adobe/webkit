#include "compiler/DependencyGraphOutput.h"

void TDepGraphOutput::outputIndentation()
{
    for (int i = 0; i < mDepth; i++)
        mSink << "  ";
}

void TDepGraphOutput::visitDepArgument(TDepArgument* depParameter)
{
    outputIndentation();
    mSink << "argument " << depParameter->getArgumentNumber() << " of call to " << depParameter->getIntermFunctionCall()->getName().c_str() << "\n";
}

void TDepGraphOutput::visitFunctionCall(TDepFunctionCall* depFunctionCall)
{
    outputIndentation();
    mSink << "function call " <<  depFunctionCall->getIntermFunctionCall()->getName().c_str() << "\n";
}

void TDepGraphOutput::visitDepSymbol(TDepSymbol* depSymbol)
{
    outputIndentation();
    mSink << depSymbol->getIntermSymbol()->getSymbol().c_str() << " (symbol id: " << depSymbol->getIntermSymbol()->getId() << ")\n";
}

void TDepGraphOutput::visitDepSelection(TDepSelection* depSelection)
{
    outputIndentation();
    mSink << "selection\n";
}

void TDepGraphOutput::visitDepLoop(TDepLoop* depLoop)
{
    outputIndentation();
    mSink << "loop condition\n";
}

void TDepGraphOutput::visitDepLogicalOp(TDepLogicalOp* depLogicalOp)
{
    outputIndentation();
    mSink << "logical " << depLogicalOp->getOpString() << "\n";
}

void TDepGraphOutput::outputAllSpanningTrees(TDepGraph* depGraph)
{
    mSink << "\n";

    TSymbolIdDepSymbolMap& symbolIdDepSymbolMap = depGraph->getSymbolIdDepSymbolMap();
    for (TSymbolIdDepSymbolMap::iterator i = symbolIdDepSymbolMap.begin(); i != symbolIdDepSymbolMap.end(); i++)
    {
        TSymbolIdDepNodePair pair = *i;
        TDepSymbol* depSymbol = pair.second;
        mSink << "--- Dependency graph spanning tree beginning at symbol " <<  depSymbol->getIntermSymbol()->getSymbol().c_str() << " ---\n";
        depSymbol->traverse(this);
        mSink << "\n";
        mVisitedDepSet.clear(); // TODO: Do something else for this.
    }
}
