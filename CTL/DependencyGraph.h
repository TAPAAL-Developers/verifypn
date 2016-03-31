#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include "../PetriEngine/PetriNet.h"
#include "../PetriParse/PNMLParser.h"
#include "../CTLParser/CTLParser.h"
#include <list>
#include <iostream>

namespace ctl{

class Configuration;
class Edge;

class DependencyGraph
{
public:

    DependencyGraph(PetriEngine::PetriNet *t_net,
                    PetriEngine::MarkVal *t_initial,
                    PNMLParser::InhibitorArcList inhibitorArcs);

    virtual ~DependencyGraph(){};

    virtual std::list<Edge*> successors(Configuration &v) =0;
    virtual Configuration &initialConfiguration()=0;
    virtual void clear(bool t_clear_all = false) =0;
    virtual int configuration_count() =0;
    virtual int marking_count() =0;

    void initialize(CTLTree &t_query);

protected:
    CTLTree *_query;
    PetriEngine::PetriNet *_petriNet;
    PetriEngine::MarkVal *_initialMarking;
    PNMLParser::InhibitorArcList _inhibitorArcs;
    int _nplaces;
    int _ntransitions;
};
}
#endif // DEPENDENCYGRAPH_H
