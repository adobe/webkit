#ifndef COMPILER_DEPENDENCY_GRAPH_H
#define COMPILER_DEPENDENCY_GRAPH_H

#include "compiler/intermediate.h"

#include <set>
#include <stack>

using namespace std;

class TDepNode;
class TDepParent;
class TDepArgument;
class TDepFunctionCall;
class TDepSymbol;
class TDepSelection;
class TDepSetStack;
class TDepLoop;
class TDepLogicalOp;
class TDepGraphTraverser;
class TDepGraphOutput;

typedef set<TDepNode*> TDepSet;
typedef set<TDepParent*> TDepParentSet;

typedef TMap<int, TDepSymbol*> TSymbolIdDepSymbolMap;
typedef pair<int, TDepSymbol*> TSymbolIdDepNodePair;

typedef TMap<TString, TDepSymbol*> TSymbolNameDepSymbolMap;
typedef pair<TString, TDepSymbol*> TSymbolNameDepSymbolPair;

typedef stack<TDepSymbol*> TDepSymbolStack;
typedef stack<bool> TBoolStack;

typedef vector<TDepNode*> TDepNodeVector;
typedef vector<TDepFunctionCall*> TDepFunctionCallVector;


// TDepNode

class TDepNode
{
public:
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
protected:
};

class TDepParent : public TDepNode
{
public:
    void addDepNode(TDepNode* depNode) { if(depNode != this) mDepSet.insert(depNode); }
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
private:
    TDepSet mDepSet;
};


// TDepArgument

class TDepArgument : public TDepParent
{
public:
    TDepArgument(TIntermAggregate* intermFunctionCall, int argumentNumber) : mIntermFunctionCall(intermFunctionCall), mArgumentNumber(argumentNumber) {}
    TIntermAggregate* getIntermFunctionCall() { return mIntermFunctionCall; }
    int getArgumentNumber() { return mArgumentNumber; }
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
protected:
    TIntermAggregate* mIntermFunctionCall;
    int mArgumentNumber;
};


// TDepFunctionCall

class TDepFunctionCall : public TDepParent
{
public:
    TDepFunctionCall(TIntermAggregate* intermFunctionCall) : mIntermFunctionCall(intermFunctionCall) {}
    TIntermAggregate* getIntermFunctionCall() { return mIntermFunctionCall; }
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
protected:
    TIntermAggregate* mIntermFunctionCall;
};


// TDepSymbol

class TDepSymbol : public TDepParent
{
public:
    TDepSymbol() : mIntermSymbol(NULL) {}    // TODO: Get rid of this.
    TDepSymbol(TIntermSymbol* intermSymbol) : mIntermSymbol(intermSymbol) {}
    TIntermSymbol* getIntermSymbol() { return mIntermSymbol; }
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
protected:
    TIntermSymbol* mIntermSymbol;
};


// TDepSelection

class TDepSelection : public TDepNode
{
public:
    TDepSelection(TIntermSelection* intermSelection) : mIntermSelection(intermSelection) {}
    TIntermSelection* getIntermSelection() { return mIntermSelection; }
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
protected:
    TIntermSelection* mIntermSelection;
};


// TDepLoop

class TDepLoop : public TDepNode
{
public:
    TDepLoop(TIntermLoop* intermLoop) : mIntermLoop(intermLoop) {}
    TIntermLoop* getIntermLoop() { return mIntermLoop; }
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
protected:
    TIntermLoop* mIntermLoop;
};


// TDepLogicalOp

class TDepLogicalOp : public TDepNode
{
public:
    TDepLogicalOp(TIntermBinary* intermLogicalOp) : mIntermLogicalOp(intermLogicalOp) {}
    TIntermBinary* getIntermLogicalOp() { return mIntermLogicalOp; }
    const char* getOpString();
    virtual void traverse(TDepGraphTraverser* depGraphTraverser);
protected:
    TIntermBinary* mIntermLogicalOp;
};


// TDepGraph

class TDepGraph
{
public:
    ~TDepGraph();
    TSymbolIdDepSymbolMap& getSymbolIdDepSymbolMap() { return mSymbolIdDepSymbolMap; }
    TSymbolNameDepSymbolMap& getGlobalScopeSymbolNameDepSymbolMap() { return mGlobalScopeSymbolNameDepSymbolMap; }
    TDepFunctionCallVector& getUserDefinedDepFunctionCalls() { return mUserDefinedDepFunctionCalls; }
    
    TDepArgument* createDepArgument(TIntermAggregate* intermFunctionCall, int argumentNumber);
    TDepFunctionCall* createDepFunctionCall(TIntermAggregate* intermFunctionCall);
    TDepSymbol* createDepSymbol(TIntermSymbol* intermSymbol);
    TDepSelection* createDepSelection(TIntermSelection* intermSelection);
    TDepLoop* createDepLoop(TIntermLoop* intermLoop);
    TDepLogicalOp* createDepLogicalOp(TIntermBinary* intermLogicalOp);
protected:
    TSymbolIdDepSymbolMap mSymbolIdDepSymbolMap;
    TSymbolNameDepSymbolMap mGlobalScopeSymbolNameDepSymbolMap;
    TDepFunctionCallVector mUserDefinedDepFunctionCalls;
    TDepNodeVector mAllDepNodes;
};


// TDepGraphTraverser

class TDepGraphTraverser
{
public:
    TDepGraphTraverser() : mDepth(0) {}
    virtual void visitDepSymbol(TDepSymbol* depSymbol) {};
    virtual void visitDepArgument(TDepArgument* depSelection) {};
    virtual void visitFunctionCall(TDepFunctionCall* depFunctionCall) {};
    virtual void visitDepSelection(TDepSelection* depSelection) {};
    virtual void visitDepLoop(TDepLoop* depLoop) {};
    virtual void visitDepLogicalOp(TDepLogicalOp* depLogicalOp) {};
    void incrementDepth() { mDepth++; }
    void decrementDepth() { mDepth--; }
    void markDepNodeVisited(TDepNode* depNode) { mVisitedDepSet.insert(depNode); }
    bool isDepNodeVisited(TDepNode* depNode) { return mVisitedDepSet.find(depNode) != mVisitedDepSet.end(); }
protected:
    int mDepth;
    TDepSet mVisitedDepSet;
};


// TDepSetStack

// TODO: Change base class inheritance to protected.
class TDepSetStack : public stack<TDepParentSet*>
{
public:
    ~TDepSetStack()
    {
        clear();
    }
    
    void pushSet()
    {
        push(new TDepParentSet());        
    }
    
    void popSet()
    {
        delete top();
        pop();
    }
    
    void combineTopTwoSets()
    {
        if (size() < 2)
            return;
        
        TDepParentSet* oldTopSet = top();
        pop();
        
        TDepParentSet* newTopSet = top();
        newTopSet->insert(oldTopSet->begin(), oldTopSet->end());

        delete oldTopSet;
    }
    
    TDepParentSet& topSet()
    {
        ASSERT(top());
        return *top();
    }
    
    void insertIntoTopSet(TDepParent* depParent)
    {
        if (empty())
            return;
        
        top()->insert(depParent);
    }
    
    void clear()
    {
        while (!empty())
            popSet();
    }
};

#endif
