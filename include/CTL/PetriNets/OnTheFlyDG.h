#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include <functional>
#include <stack>
#include <ptrie/ptrie_map.h>

#include "CTL/DependencyGraph/BasicDependencyGraph.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/DependencyGraph/Edge.h"
#include "PetriConfig.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/Structures/AlignedEncoder.h"
#include "PetriEngine/Structures/linked_bucket.h"
#include "PetriEngine/ReducingSuccessorGenerator.h"

namespace PetriNets {
class OnTheFlyDG : public DependencyGraph::BasicDependencyGraph
{
public:
    using Condition = PetriEngine::PQL::Condition;
    using Condition_ptr = PetriEngine::PQL::Condition_ptr;
    using Marking = PetriEngine::Structures::State;
    OnTheFlyDG(const PetriEngine::PetriNet& t_net, bool partial_order);

    virtual ~OnTheFlyDG();

    //Dependency graph interface
    virtual std::vector<DependencyGraph::Edge*> successors(DependencyGraph::Configuration *c) override;
    virtual DependencyGraph::Configuration *initialConfiguration() override;
    virtual void cleanUp() override;
    void setQuery(const Condition_ptr& query);

    virtual void release(DependencyGraph::Edge* e) override;

    size_t owner(Marking& marking, Condition* cond);
    size_t owner(Marking& marking, const Condition_ptr& cond)
    {
        return owner(marking, cond.get());
    }


    //stats
    size_t configurationCount() const;
    size_t markingCount() const;

    Condition::Result initialEval();

protected:

    //initialized from constructor
    const PetriEngine::PetriNet& _net;
    AlignedEncoder _encoder;
    PetriConfig* _initial_config;
    Marking _working_marking;
    Marking _query_marking;
    size_t _markingCount = 0;
    size_t _configurationCount = 0;
    //used after query is set
    Condition_ptr _query = nullptr;

    Condition::Result fast_eval(Condition* query, Marking* unfolded);
    Condition::Result fastEval(const Condition_ptr& query, Marking* unfolded)
    {
        return fast_eval(query.get(), unfolded);
    }
    void next_states(Marking& t_marking, Condition*,
    std::function<void ()> pre,
    std::function<bool (Marking&)> foreach,
    std::function<void ()> post);

    template<typename T>
    void do_work(T& gen, bool& first,
    std::function<void ()>& pre,
    std::function<bool (Marking&)>& foreach)
    {
        gen.prepare(_query_marking);

        while(gen.next(_working_marking)){
            if(first) pre();
            first = false;
            if(!foreach(_working_marking))
            {
                gen.reset();
                break;
            }
        }
    }
    PetriConfig *create_configuration(size_t marking, size_t own, Condition* query);
    PetriConfig *create_configuration(size_t marking, size_t own, const Condition_ptr& query)
    {
        return create_configuration(marking, own, query.get());
    }
    size_t create_marking(Marking &marking);
    void marking_stats(const uint32_t* marking, size_t& sum, bool& allsame, uint32_t& val, uint32_t& active, uint32_t& last);

    DependencyGraph::Edge* new_edge(DependencyGraph::Configuration &t_source, uint32_t weight);

    std::stack<DependencyGraph::Edge*> _recycle;
    ptrie::map<ptrie::uchar, std::vector<PetriConfig*> > _trie;
    linked_bucket_t<DependencyGraph::Edge,1024*10>* _edge_alloc = nullptr;

    // Problem  with linked bucket and complex constructor
    linked_bucket_t<char[sizeof(PetriConfig)], 1024*1024>* _conf_alloc = nullptr;

    PetriEngine::ReducingSuccessorGenerator _redgen;
    bool _partial_order = false;

};


}
#endif // ONTHEFLYDG_H
