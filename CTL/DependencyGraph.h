#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include "../PetriEngine/PetriNet.h"
#include "../CTLParser/CTLParser.h"
#include <list>

namespace ctl{

class Configuration;
class Edge;

class DependencyGraph
{
public:

    DependencyGraph(PetriEngine::PetriNet *t_net,
                    PetriEngine::MarkVal *t_initial)
        : _petriNet(t_net),
          _initialMarking(t_initial),
          _nplaces(t_net->numberOfPlaces()),
          _ntransitions(t_net->numberOfTransitions())
    {}

    virtual ~DependencyGraph(){};

    virtual std::list<Edge*> successors(Configuration &v) =0;
    virtual Configuration &initialConfiguration()=0;
    virtual void clear(bool t_clear_all = false) =0;
    virtual int configuration_count() =0;
    virtual int marking_count() =0;

    DependencyGraph *initialize(CTLTree &t_query);

    bool querySatisfied() const;
    void setQuerySatisfied(bool querySatisfied);

protected:
    bool _querySatisfied;
    CTLTree *_query = NULL;
    PetriEngine::PetriNet *_petriNet = NULL;
    PetriEngine::MarkVal *_initialMarking = NULL;
    int _nplaces;
    int _ntransitions;
};
}
#endif // DEPENDENCYGRAPH_H
