#include "CTL/Algorithm/RankCertainZeroFPA.h"
#include "CTL/PetriNets/OnTheFlyDG.h"
#include "CTL/DependencyGraph/Configuration.h"

#include <cassert>
#include <iostream>

#include "CTL/SearchStrategy/DFSSearch.h"

using namespace DependencyGraph;
using namespace SearchStrategy;
using namespace Algorithm;

#ifndef NDEBUG
#define ASSERT(x) assert(x)
#define DEBUG_ONLY(x) x
#else
#define ASSERT(x) if (!(x)) { std::cerr << "Assertion " << #x << " failed\n"; exit(1); }
#define DEBUG_ONLY(x)
#endif

DEBUG_ONLY(static void print_edge(Edge* e) {
    std::cerr << '(' << e->source->id << "r" << e->source->rank;
    if (e->is_negated) {
        std::cerr << " -- ";
    } else {
        std::cerr << ", { ";
    }
    for (auto c: e->targets) {
        std::cerr << c->id << "r" << c->rank << "(" << c->min_rank << "):" << to_string((Assignment)c->assignment) << ' ';
    }
    std::cerr << (e->is_negated ? ")" : "})");
})
#ifndef NDEBUG
std::map<std::string,std::string> correct;

std::pair<std::string,std::string> split(const std::string& line) {
    for(int64_t i = int64_t{line.size()}-1; i > 0; --i)
    {
        if(line[i] == ',')
            return std::make_pair(std::string(line.data(), i), std::string(line.data() + i, (line.size() - i)));
    }
    return {"",""};
}
#endif

void RankCertainZeroFPA::set_assignment(Configuration *c, Assignment a) {
    assert(!c->isDone());
    c->assignment = a;
    if(c->isDone())
    {
        for(auto* e : c->successors)
        {
            --e->refcnt;
            if(e->refcnt == 0)
                graph->release(e);
        }
        c->successors.clear();
    }
#ifndef NDEBUG
    if(correct.empty())
    {
        std::fstream in;
        in.open("/home/pgj/Devel/verifypn/verifypn-git/build/assign", std::ios::in);
        if(in)
        {
            std::string line;
            while(getline(in, line)){
                auto [a, b] = split(line);
                correct[a] = b;
            }
        }
    }
    //DEBUG_ONLY(std::cerr << "ASSIGN: [" << c->id << "]: " << to_string(static_cast<Assignment>(c->assignment)) << "\n";)
    if(c->isDone())
    {
        std::stringstream ss;
        graph->print(c, ss);
        auto [a,b] = split(ss.str());
        if(correct.count(a))
        {
            if(correct[a] != b)
            {
                std::cerr << "Error on :" << a << "\n\tGOT" << b << " expected " << correct[a] << std::endl;
                assert(false);
            }
            /*else
                std::cerr << "¤ OK (" << c->id << ") = " << DependencyGraph::to_string((Assignment)c->assignment) << std::endl;*/
        }
//        else std::cerr << "¤ NO MATCH (" << c->id << ") " << DependencyGraph::to_string((Assignment)c->assignment) << std::endl;
    }
#endif
}

static auto rng = std::default_random_engine {};
Algorithm::RankCertainZeroFPA::RankCertainZeroFPA(Strategy type) : FixedPointAlgorithm(), _strategy(type) {
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

bool Algorithm::RankCertainZeroFPA::search(DependencyGraph::BasicDependencyGraph& t_graph) {
    auto res = _search(t_graph);
    return res;
}

void Algorithm::RankCertainZeroFPA::expand(Configuration* config) {
    if(!config->successors.empty())
        return;
    auto succs = graph->successors(config);
    if(_strategy == Strategy::DFS)
    {
        std::reverse(succs.begin(), succs.end());
    }
    else if(_strategy == Strategy::BFS)
    {
        // nothing
    }
    else if(_strategy == Strategy::RDFS || _strategy == Strategy::HEUR)
    {
        std::shuffle(succs.begin(), succs.end(), rng);
    }
    ++_exploredConfigurations;
    for(auto* e : succs) // use reverse iterator to preserve order
    {
        bool has_czero = false;
        bool all_one = true;
        e->refcnt = 1;
        for(Configuration* t : e->targets)
        {
            all_one &= t->assignment == ONE;
            if(t->assignment == CZERO)
            {
                has_czero = true;
                break;
            }
            t->addDependency(e);
        }

        if((!e->is_negated && all_one) && (e->is_negated && has_czero))
                e->targets.clear();
        else if((!e->is_negated && has_czero) || (e->is_negated && all_one))
        {
            --e->refcnt;
            if(e->refcnt == 0)
                graph->release(e);
        }
        else
            config->successors.emplace_front(e);
    }
}

void Algorithm::RankCertainZeroFPA::push_to_wating(DependencyGraph::Configuration* config, wstack_t& waiting)
{
    ++_max_rank;
    config->min_rank = config->rank = _max_rank;
    config->min_rank_source = config;
    expand(config);
    waiting.emplace_back(config);
    config->on_stack = true;
    config->assignment = ZERO;
}

void Algorithm::RankCertainZeroFPA::do_pop(wstack_t& waiting, DependencyGraph::Configuration* lowest) {
    if(lowest != nullptr)
    {
        while(waiting.back() != lowest)
        {
            waiting.back()->on_stack = false;
            waiting.pop_back();
        }
    }
    auto* conf = waiting.back();
    conf->on_stack = false;
    waiting.pop_back();
}

DependencyGraph::Configuration* Algorithm::RankCertainZeroFPA::find_undecided(Configuration* conf) {
    auto& edges = conf->successors;
    Configuration* undecided = nullptr;
    bool all_czero = true;
    assert(conf->min_rank == conf->rank);
    auto pre = edges.before_begin();
    auto it = edges.begin();
    while(it != edges.end())
    {
        auto* e = *it;
        auto [ud, val] = eval_edge(e);
        if(val == ONE)
        {
            set_assignment(conf, ONE);
            all_czero = false;
            break;
        }
        else if(val == CZERO)
        {
            assert(e->refcnt >= 1);
            --e->refcnt;
            if(e->refcnt == 0)
                graph->release(e);
            edges.erase_after(pre);
            it = pre;
        }
        // if edge had undecided and we couldn't assign, select that one as next conf.
        else {
            if(ud != nullptr && (
                    undecided == nullptr ||
                    // we need to explore any edge which has only unexplored with priority.
                    (undecided->assignment == ZERO && ud->assignment == UNKNOWN) ||
                    // this mimicks the behaviour of the search of CZERO algorithm, pick the *last* successor of the last edge to explore first
                    (undecided->assignment == ud->assignment &&
                      ( ( ( undecided->successors.empty() || !ud->successors.empty()) && _strategy == Strategy::DFS) || // prefer undecided that were already explored
                        ( (!undecided->successors.empty() ||  ud->successors.empty()) && _strategy == Strategy::BFS)) // prefer undecided that were NOT already explored
                        // otherwise follow the search order
                    )))
            {
                undecided = ud;
            }
            all_czero = false;
        }
        pre = it;
        ++it;
        if(conf->isDone())
            break;
    }
    if(all_czero)
        set_assignment(conf, CZERO);
    return undecided;
}

DependencyGraph::Configuration* Algorithm::RankCertainZeroFPA::handle_early_czero(Configuration* conf) {
    Configuration* bot = nullptr;
    auto min_rank = conf->rank;
    auto min_max_rank = conf->rank;
    Configuration* mr_source = conf;
    assert(min_rank > 0);
    // if the largest rank of any target is in the sub-graph (which
    // needs nothing from the stack as value currently), we can conclude
    // that the edge itself will never reach a verdict, and we can
    // already determine it CZERO (or ONE in the case of negation).
    bool all_ref = true;
    for (auto* e : conf->successors) {
        assert(eval_edge(e).second == ZERO);
        if (e->is_negated) // we know it is determined already
        {
            set_assignment(conf, ONE);
            bot = backprop(conf);
            break;
        }

        // technically we *could* look at a min (over edges) and max (over targets), however, targets
        // might change their maximal min_val of their edges post pop.
        // This happens when a ONE is propagated in the backup -- which
        // can lead to the min_val value of a target *not* being correct
        // at the time we execute the following. At least this is my
        // working hypothesis. The propagation of ONE can lead to a
        // new target being the maximal -- possibly with a min_rank lower
        // than that of the current config.
        decltype(min_max_rank) tmp_max = 0;
        decltype(min_max_rank) tmp_min = min_rank;
        Configuration* tmp_min_src = mr_source;
        bool any = false;
        bool any_ref = false;
        for (auto* t : e->targets) {
            any = true;
            if(t == conf || t->min_rank_source == conf)
            {
                any_ref = true;
                break;
            }
            if (!t->isDone() && t->min_rank != 0) {
                if (t->min_rank < tmp_min) {
                    if (t->min_rank_source) {
                        tmp_min_src = t->min_rank_source;
                        assert(t->min_rank_source->on_stack);
                        assert(t->min_rank_source->rank == t->min_rank);
                    } else {
                        tmp_min_src = t;
                        assert(t->min_rank == t->rank);
                        assert(t->on_stack);
                    }
                    tmp_min = t->min_rank;
                }
                tmp_max = std::max(tmp_max, t->min_rank);
            }
        }

        all_ref &= any_ref;
        if (any)
            min_max_rank = std::min(tmp_max, min_max_rank);
        if(!any_ref && any) // if the edge is dependent on the node itself (any_ref), then we do not have to take the rank of the edge into account.
                     // it will be blocked by (conf) regardless
        {
            assert(tmp_min_src); // otherwise this edge would be a ONE edge (or ZERO if negated)
            min_rank = tmp_min;
            mr_source = tmp_min_src;
        }
    }
    if (!conf->isDone()) {
        assert(min_max_rank <= conf->rank);
        if (min_max_rank >= conf->rank || all_ref) // all branches are waiting for somebody later in the tree, we can safely conclude.
        {
            // well, supposedly this will imply that min_max_rank == conf->rank; namely that the current TOS
            // actually is an infliction-point of all successors
            if(!_early_output)
                std::cerr << "EARLY_CZERO" << std::endl;
            _early_output = true;
            set_assignment(conf, CZERO);
            auto* b = backprop(conf);
            if (bot == nullptr || b->rank < bot->rank)
                bot = b;
        } else {
            conf->min_rank = min_rank;
            conf->min_rank_source = mr_source;
        }
    }
    return bot;
}

bool Algorithm::RankCertainZeroFPA::_search(DependencyGraph::BasicDependencyGraph& t_graph) {
    graph = &t_graph;

    root = graph->initialConfiguration();
    wstack_t waiting;
    push_to_wating(root, waiting);

    while(!waiting.empty() && !root->isDone())
    {
        auto* conf = waiting.back();
        if(conf->isDone()) {
            auto* c = backprop(conf);
            assert(c == conf);
            do_pop(waiting, c);
            continue;
        }

        auto* undecided = find_undecided(conf);

        if(conf->isDone()) {
            auto* bot = backprop(conf);
            do_pop(waiting, bot);
            continue;
        }

        if(undecided == nullptr || undecided->assignment != UNKNOWN) {
            // either nobody is undecided (all have values) or at least they are unexplored
            // in the later case we can *maybe* assign a czero here.
            auto* bot = handle_early_czero(conf);
            do_pop(waiting, bot);
            continue;
        }
        else
            push_to_wating(undecided, waiting);
    }

    return root->assignment == ONE;
}


std::pair<Configuration *, Assignment> Algorithm::RankCertainZeroFPA::eval_edge(DependencyGraph::Edge *e) {
    bool allOne = true, hasCZero = false;
    Configuration *retval = nullptr;
    auto it = e->targets.begin();
    auto pit = e->targets.before_begin();
    while (it != e->targets.end()) {
        // need seq-number
        // if target node has min_rank from an unassigned node that is either outside the stack or has a different min_rank,
        // and it is not a self-loop, then wipe information of that target node.
        if ((*it)->min_rank_source &&
            (!(*it)->min_rank_source->on_stack || (*it)->min_rank_source->rank != (*it)->min_rank) &&
            (*it) != e->source && !(*it)->isDone())
        {
            // the minrank/anchor is not on the stack (or was popped and readded) so we cannot trust the rank, thus we set the minrank to unknown (0).
            (*it)->min_rank = 0;
            (*it)->rank = 0;
            (*it)->assignment = UNKNOWN;
        }
        if(*it == e->source)
        {
            assert(!e->is_negated);
            hasCZero = true; // trivial self-loop
            allOne = false;
            break;
        }
        else if ((*it)->assignment == ONE) {
            e->targets.erase_after(pit);
            it = pit;
        } else {
            allOne = false;
            if ((*it)->assignment == CZERO) {
                hasCZero = true;
                break;
            }
            else if(e->is_negated && (*it)->assignment == ZERO)
            {
                // holds due to DFS + acyclic negation
                hasCZero = true;
                break;
            } else if (retval == nullptr ||
                    (retval->assignment == UNKNOWN && (*it)->assignment == ZERO) ||
                    (retval->assignment == (*it)->assignment && retval->rank < (*it)->rank)) {
                retval = *it;
            }
        }
        pit = it;
        ++it;
    }

    if(allOne)
        return std::make_pair(nullptr, e->is_negated ? CZERO : ONE);
    if(hasCZero)
        return std::make_pair(nullptr, e->is_negated ? ONE : CZERO);
    else if(retval)
        return std::make_pair(retval, (Assignment)retval->assignment);
    else
    {
        assert(false);
        return std::make_pair(nullptr, UNKNOWN);
    }
}

void Algorithm::RankCertainZeroFPA::backprop_edge(Edge* edge) {
    assert(false);
}

Configuration* Algorithm::RankCertainZeroFPA::backprop(Configuration* source) {
    assert(source->isDone());
    std::stack<Configuration*> waiting;
    waiting.emplace(source);
    Configuration* min = source;
    while (!waiting.empty()) {
        auto* conf = waiting.top();
        assert(conf->isDone());
        if(conf->rank < min->rank && conf->on_stack)
            min = conf;
        waiting.pop();
        auto prev = conf->dependency_set.before_begin();
        auto cur = conf->dependency_set.begin();
        while(cur != conf->dependency_set.end())
        {
            Edge* e = *cur;
            auto* c = e->source;
            if(c->isDone()) {
                --e->refcnt;
                if(e->refcnt == 0)
                    graph->release(e);
                conf->dependency_set.erase_after(prev);
                cur = prev;
                ++cur;

                continue;
            }
            auto [und, assignment] = eval_edge(e);

            if(assignment == ONE) {
                set_assignment(c, ONE);
                waiting.push(c);
            }
            else if(assignment == UNKNOWN) {
                // nothing.
            }
            else {
                // this could probably be done faster by exploiting "nsuccs"
                bool all_zero = true;
                for(auto* e : c->successors)
                {
                    auto r = eval_edge(e);
                    all_zero &= r.second == CZERO;
                }
                if(all_zero) {
                    set_assignment(c, CZERO);
                    waiting.push(c);
                }
            }
            if(!c->isDone())
            {
                bool all_ref = true;
                for(auto* e : c->successors)
                {
                    auto res = eval_edge(e);
                    if(res.second != CZERO && res.second != ONE)
                    {
                        assert(res.second != ONE);
                        bool some_ref = false;
                        for(auto* t : e->targets)
                        {
                            /*if(t->min_rank_source &&
                                    t->min_rank_source->on_stack && t->min_rank_source->min_rank == t->min_rank)
                                mx = std::min(mx, t->min_rank);
                             */
                            if(t->min_rank_source == c)
                            {
                                some_ref = true;
                                break;
                            }
                        }
                        if(!some_ref)
                        {
                            all_ref = false;
                            break;
                        }
                    }
                }
                if(all_ref)
                {
                    // I suspect this *never* happens. The earlier check during forward
                    // exploration would essentially detect this. However due to
                    // some edges getting assigned CZERO, this check *could* potentially
                    // kick in, as the remaining form a loop.
                    // TODO, figure out if we can do this in more cases?
                    set_assignment(c, CZERO);
                    waiting.push(c);
                    if(!_backloop_output)
                        std::cerr << "BACKLOOP" << std::endl;
                    _backloop_output = true;
                }
            }
            {
                --e->refcnt;
                if(e->refcnt == 0)
                    graph->release(e);
                conf->dependency_set.erase_after(prev);
            }
            cur = prev;
            ++cur;
        }
    }
    return min;
}
