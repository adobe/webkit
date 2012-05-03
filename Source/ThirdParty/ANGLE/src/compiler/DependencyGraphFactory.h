#ifndef COMPILER_DEPENDENCY_GRAPH_FACTORY_H
#define COMPILER_DEPENDENCY_GRAPH_FACTORY_H

#include "compiler/DependencyGraph.h"

// TDepGraphFactory

class TDepGraphFactory : public TIntermTraverser
{
public:
    TDepGraphFactory() : TIntermTraverser(true, false, false), mDepGraph(NULL) {}

    TDepGraph* createDepGraph(TIntermNode* node);

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit visit, TIntermBinary*);
    virtual bool visitUnary(Visit visit, TIntermUnary*);
    virtual bool visitSelection(Visit visit, TIntermSelection*);
    virtual bool visitAggregate(Visit visit, TIntermAggregate*);
    virtual bool visitLoop(Visit visit, TIntermLoop*);
    virtual bool visitBranch(Visit visit, TIntermBranch*);

private:
    TDepSymbol* getOrCreateDepSymbolByIntermSymbol(TIntermSymbol* node);

    void connectDepSetToDepNode(TDepParentSet& depNodeSet, TDepNode* depNode);

    void visitAssignment(TIntermBinary* node);
    void visitLogicalOp(TIntermBinary* node);
    void visitBinaryChildren(TIntermBinary* node);
    void visitFunctionDefinition(TIntermAggregate* node);
    void visitFunctionCall(TIntermAggregate* node);
    void visitAggregateChildren(TIntermAggregate* node);

    TDepGraph* mDepGraph;
    TDepSetStack mDepSetStack;
    TBoolStack mIsUnderLeftSubTreeStack;
    TDepSymbolStack mLeftmostDepSymbolStack;
    bool mIsGlobalScope;

    TDepSymbol NULL_DEP_SYMBOL; // TODO: Want something more elegant.
};

#endif
