#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include <functional>
#include <ptrie/ptrie_map.h>
#include <stack>

#include "CTL/DependencyGraph/BasicDependencyGraph.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/DependencyGraph/Edge.h"
#include "PetriConfig.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/ReducingSuccessorGenerator.h"
#include "PetriEngine/Structures/AlignedEncoder.h"
#include "PetriEngine/Structures/linked_bucket.h"
#include "PetriParse/PNMLParser.h"

namespace PetriNets {
class OnTheFlyDG : public DependencyGraph::BasicDependencyGraph {
  public:
    using Condition = PetriEngine::PQL::Condition;
    using Condition_ptr = PetriEngine::PQL::Condition_ptr;
    using Marking = PetriEngine::Structures::State;
    OnTheFlyDG(const PetriEngine::PetriNet &t_net, bool partial_order);

    virtual ~OnTheFlyDG();

    // Dependency graph interface
    virtual std::vector<DependencyGraph::Edge *>
    successors(DependencyGraph::Configuration *c) override;
    virtual DependencyGraph::Configuration *initial_configuration() override;
    void cleanup();
    void set_query(const Condition_ptr &query);

    virtual void release(DependencyGraph::Edge *e) override;

    // stats
    size_t configuration_count() const;
    size_t marking_count() const;

    Condition::Result initial_eval();

  protected:
    // initialized from constructor
    const PetriEngine::PetriNet &_net;
    AlignedEncoder _encoder;
    PetriConfig *_initial_config;
    Marking _working_marking;
    Marking _query_marking;
    size_t _markingCount = 0;
    size_t _configurationCount = 0;
    // used after query is set
    Condition_ptr _query = nullptr;

    Condition::Result fast_eval(Condition *query, Marking *unfolded);
    Condition::Result fast_eval(const Condition_ptr &query, Marking *unfolded) {
        return fast_eval(query.get(), unfolded);
    }
    void next_states(Marking &t_marking, Condition *, std::function<void()> &&pre,
                     std::function<bool(Marking &)> &&foreach, std::function<void()> &&post);

    template <typename T>
    void do_work(T &gen, bool &first, std::function<void()> &&pre,
                 std::function<bool(Marking &)> &&foreach) {
        gen.prepare(_query_marking);

        while (gen.next(_working_marking)) {
            if (first)
                pre();
            first = false;
            if (!foreach (_working_marking)) {
                gen.reset();
                break;
            }
        }
    }
    PetriConfig *create_configuration(size_t marking, Condition *query);
    PetriConfig *create_configuration(size_t marking, const Condition_ptr &query) {
        return create_configuration(marking, query.get());
    }
    size_t create_marking(Marking &marking);

    DependencyGraph::Edge *new_edge(DependencyGraph::Configuration &t_source, uint32_t weight);

    std::stack<DependencyGraph::Edge *> _recycle;
    ptrie::map<ptrie::uchar, std::vector<PetriConfig *>> _trie;
    linked_bucket_t<DependencyGraph::Edge, 1024 * 10> *_edge_alloc = nullptr;

    // Problem  with linked bucket and complex constructor
    linked_bucket_t<char[sizeof(PetriConfig)], 1024 * 1024> *_conf_alloc = nullptr;

    PetriEngine::ReducingSuccessorGenerator _redgen;
    bool _partial_order = false;
};

} // namespace PetriNets
#endif // ONTHEFLYDG_H
