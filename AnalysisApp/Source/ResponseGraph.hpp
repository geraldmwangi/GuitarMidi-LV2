#pragma once
#include <Graph.hpp>

class ResponseGraph: public Graph
{
    protected:
    virtual void computeGraph();
    public:
    ResponseGraph(shared_ptr<NoteClassifier> notecl);
    virtual ~ResponseGraph();
};