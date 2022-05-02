#pragma once
#include <Graph.hpp>
#include <JuceHeader.h>

class ResponseGraph : public Graph
{
protected:
    virtual void computeGraph();

public:
    ResponseGraph(shared_ptr<NoteClassifier> notecl);
    virtual ~ResponseGraph();
};