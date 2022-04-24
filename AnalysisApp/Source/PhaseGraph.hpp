#pragma once
#include <Graph.hpp>

class PhaseGraph : public Graph
{
protected:
    virtual void computeGraph();

public:
    PhaseGraph(shared_ptr<NoteClassifier> notecl);
    virtual ~PhaseGraph();
};