#include "compiler/DependencyGraph.h"

void TDepNode::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->markDepNodeVisited(this);
}

void TDepParent::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->markDepNodeVisited(this);

    depGraphTraverser->incrementDepth();

    for (TDepSet::iterator i = mDepSet.begin(); i != mDepSet.end(); i++)
    {
        TDepNode* depNode = *i;
        if (!depGraphTraverser->isDepNodeVisited(depNode))
            depNode->traverse(depGraphTraverser);
    }

    depGraphTraverser->decrementDepth();
}

void TDepArgument::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->visitDepArgument(this);
    TDepParent::traverse(depGraphTraverser);
}

void TDepFunctionCall::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->visitFunctionCall(this);
    TDepParent::traverse(depGraphTraverser);
}

void TDepSymbol::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->visitDepSymbol(this);
    TDepParent::traverse(depGraphTraverser);
}

void TDepSelection::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->visitDepSelection(this);
    TDepNode::traverse(depGraphTraverser);
}

void TDepLoop::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->visitDepLoop(this);
    TDepNode::traverse(depGraphTraverser);
}

void TDepLogicalOp::traverse(TDepGraphTraverser* depGraphTraverser)
{
    depGraphTraverser->visitDepLogicalOp(this);
    TDepNode::traverse(depGraphTraverser);
}
