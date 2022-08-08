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
        c->successors.clear();
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

bool is_assignable(Edge* e) {
    auto c = e->source;
    if (c->nsuccs == 0) {
        return true;
    }
    bool allOne = true;
    for (auto v: e->targets) {
        if (v->assignment == CZERO)
            return c->nsuccs == 1;
        if (v->assignment != ONE)
            return false;
    }
    return true;
}

bool Algorithm::RankCertainZeroFPA::search(DependencyGraph::BasicDependencyGraph& t_graph) {
    auto res = _search(t_graph);
    return res;
}


bool Algorithm::RankCertainZeroFPA::_search(DependencyGraph::BasicDependencyGraph& t_graph) {
    graph = &t_graph;

    root = graph->initialConfiguration();
    ++_max_rank;
    root->min_rank = root->rank = _max_rank;
    wstack_t waiting;
    waiting.emplace_back(root);
    root->successors = graph->successors(root);
    for(auto* e : root->successors)
    {
        if(e->refcnt == 1) // only in waiting
        {
            for(auto& t : e->targets)
            {
                t->addDependency(e);
            }
            //--e->refcnt;   // shouldn't be needed?
        }
        else if(e->refcnt >= 2) // already added as depender
        {
            assert(false);
        }
        else assert(false); // should never happen
    }
    root->on_stack = true;
    ++_exploredConfigurations;
    root->assignment = ZERO;
    //DEBUG_ONLY(std::cerr << "PUSH [" << root->id << "]" << std::endl;)
#ifdef DG_REFCOUNTING
    root->refc = 1;
#endif

    auto do_pop = [&waiting,this](Configuration* lowest = nullptr) {
        size_t n = 1;
        if(lowest != nullptr)
        {
            while(waiting.back() != lowest)
            {
                waiting.back()->on_stack = false;
                waiting.pop_back();
                ++n;
            }
        }
        auto* conf = waiting.back();
        conf->on_stack = false;
        /*if(n > 1)
            std::cerr << "POP " << n << std::endl;*/
        /*if(conf->isDone())
        {
            for(auto* e : edges)
            {
                e->status = DependencyGraph::EdgeStatus::NotWaiting;
                assert(e->refcnt == 1);
                e->refcnt = 0;
                graph->release(e);
            }
        }
        else
        {
            //assert(conf->nsuccs == 0);
            //conf->nsuccs = 0;
            for(auto* e : edges)
            {
                if(e->refcnt == 1) // only in waiting
                {
                    for(auto& t : e->targets)
                    {
                        t->addDependency(e);
                    }
                    //--e->refcnt;   // shouldn't be needed?
                }
                else if(e->refcnt >= 2) // already added as depender
                {
                    assert(false);
                }
                else assert(false); // should never happen
            }
        }*/
        waiting.pop_back();
    };

    while(!waiting.empty() && !root->isDone())
    {
        auto* conf = waiting.back();
        if(conf->isDone()) {
            auto* c = backprop(conf);
            assert(c == conf);
            //DEBUG_ONLY(std::cerr << "POP [" << conf->id << "r" << conf->rank << "] (162:: " << to_string((Assignment)conf->assignment) << " mr" << conf->min_rank << ")" << std::endl;)
            do_pop();
            continue;
        }
        auto& edges = conf->successors;
        Configuration* undecided = nullptr;
        Edge* undecided_edge = nullptr;
        bool all_czero = true;
        size_t j = 0;
        assert(conf->min_rank == conf->rank);
        for(size_t i = 0; i < edges.size(); ++i)
        {
            auto* e = edges[i];
            auto [ud, val] = eval_edge(e, &waiting);
            /*DEBUG_ONLY(print_edge(e);
            std::cerr << to_string(val);
            std::cerr << std::endl;)*/
            if(val == ONE)
            {
                set_assignment(conf, ONE);
                all_czero = false;
                for(size_t n = i; n < edges.size(); ++n)
                {
                    edges[j] = edges[n];
                    ++j;
                }
                break;
            }
            else if(val == CZERO)
            {
                //assert(e->refcnt == 1);
                //e->refcnt = 0;
                //graph->release(e);
                // skip
            }
            // if edge had undecided and we couldn't assign, select that one as next conf.
            else {
                edges[j] = edges[i];
                ++j;
                if(ud != nullptr && (
                        undecided == nullptr ||
                        (undecided->assignment == ZERO && ud->assignment == UNKNOWN)))
                {
                    undecided = ud;
                    undecided_edge = e;
                }
                all_czero = false;
            }

            if(conf->isDone())
                break;
        }
        if(all_czero)
        {
            //std::cerr << "ALL CZERO (O=" << edges.size() << ")" << std::endl;
            set_assignment(conf, CZERO);
        }
        edges.resize(j);

        if(conf->isDone()) {
            auto* bot = backprop(conf);
            //graph->print(conf);
            //DEBUG_ONLY(std::cerr << "POP [" << conf->id << "r" << conf->rank << "] (223:: " << to_string((Assignment)conf->assignment) << " mr" << conf->min_rank << ")" << std::endl;)
            do_pop(bot);
            continue;
        }
        Configuration* bot = nullptr;
        if(undecided == nullptr || undecided->assignment != UNKNOWN) {
            //DEBUG_ONLY(std::cerr << "POP [" << conf->id << "] (undecided == nullptr)" << std::endl;)
            auto min_rank = conf->rank;
            auto min_max_rank = conf->rank;
            Configuration* mr_source = conf;
            assert(min_rank > 0);
            // if the largest rank of any target is in the sub-graph (which
            // needs nothing from the stack as value currently), we can conclude
            // that the edge itself will never reach a verdict, and we can
            // already determine it CZERO (or ONE in the case of negation).
            for(auto* e : edges)
            {
                assert(eval_edge(e).second == ZERO);
                if(e->is_negated) // we know it is determined already
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
                decltype(min_max_rank) tmp = 0;
                bool any = false;
                for(auto* t : e->targets)
                {
                    any = true;
                    if(!t->isDone() && t->min_rank != 0)
                    {
                        if(t->min_rank < min_rank)
                        {
                            if(t->min_rank_source)
                            {
                                mr_source = t->min_rank_source;
                                assert(t->min_rank_source->on_stack);
                                assert(t->min_rank_source->rank == t->min_rank);
                            }
                            else
                            {
                                mr_source = t;
                                assert(t->min_rank == t->rank);
                                assert(t->on_stack);
                            }
                            min_rank = t->min_rank;
                        }
                        tmp = std::max(tmp, t->min_rank);
                    }
                }
                if(any)
                {
                    min_max_rank = std::min(tmp, min_max_rank);
                }
            }
            if(!conf->isDone())
            {
                assert(min_max_rank <= conf->rank);
                if(min_max_rank >= conf->rank) // all branches are waiting for somebody later in the tree, we can safely conclude.
                {
                    //std::cerr << "EARLY [" << min_max_rank << ", " << min_rank << ", " << conf->min_rank << "]" << std::endl;
/*#ifndef NDEBUG
                    for(auto* e : edges)
                        print_edge(e);
#endif
*/

                    set_assignment(conf, CZERO);
                    auto* b = backprop(conf);
                    if(bot && b->rank < bot->rank)
                        bot = b;
                }
                else
                {
                    conf->min_rank = min_rank;
                    conf->min_rank_source = mr_source;
                }
            }
//            DEBUG_ONLY(std::cerr << "POP [" << conf->id << "r" << conf->rank << "] (286:: " << to_string((Assignment)conf->assignment) << " mr" << conf->min_rank << ")" << std::endl;)
            do_pop(bot);
            continue;
        }
        else
        {
            ++_max_rank;
            undecided->min_rank = undecided->rank = _max_rank;
            undecided->assignment = ZERO;
            undecided->min_rank_source = undecided;
            if(undecided->successors.empty())
            {
                ++_exploredConfigurations;
                undecided->successors = graph->successors(undecided);
                for(auto* e : undecided->successors)
                {
                    if(e->refcnt == 1) // only in waiting
                    {
                        for(auto& t : e->targets)
                        {
                            ++e->refcnt;
                            t->addDependency(e);
                        }
                        //--e->refcnt;   // shouldn't be needed?
                    }
                    else if(e->refcnt >= 2) // already added as depender
                    {
                        assert(false);
                    }
                    else assert(false); // should never happen
                }
            }
            waiting.emplace_back(undecided);

            //DEBUG_ONLY(std::cerr << "PUSH [" << undecided->id << "]" << std::endl;)
            //graph->print(undecided, std::cerr);
            /*std::cerr << std::endl;*/
            /*DEBUG_ONLY(
                for(auto& e : waiting.back().second){
                    std::cerr << "\t";
                    print_edge(e);
                    std::cerr << "\n";
                }
                //if(waiting.back().second.empty())
                {
                    graph->print(undecided, std::cerr);
                    std::cerr << std::endl;
                }
            )*/
            undecided->on_stack = true;
        }
    }

    return root->assignment == ONE;
}

void Algorithm::RankCertainZeroFPA::checkEdge(Edge* e, bool only_assign, bool was_dep) {
    assert(false);
}

void Algorithm::RankCertainZeroFPA::finalAssign(DependencyGraph::Edge* e, DependencyGraph::Assignment a) {
    assert(false);
}

void Algorithm::RankCertainZeroFPA::finalAssign(DependencyGraph::Configuration* c, DependencyGraph::Assignment a) {
    assert(a == ONE || a == CZERO);
    assert(false);

}

std::vector<Edge*> Algorithm::RankCertainZeroFPA::explore(Configuration* c) {
    assert(false);
}


std::pair<Configuration *, Assignment> Algorithm::RankCertainZeroFPA::eval_edge(DependencyGraph::Edge *e, wstack_t* waiting) {
    bool allOne = true, hasCZero = false;
    Configuration *retval = nullptr;
    Assignment a = ZERO;
    /*DEBUG_ONLY(
        std::cerr << "EVAL [";
        print_edge(e);
        std::cerr << "]\n";
    )*/
    auto it = e->targets.begin();
    auto pit = e->targets.before_begin();
    while (it != e->targets.end()) {
        // need seq-number
        if((*it)->min_rank_source && (!(*it)->min_rank_source->on_stack || (*it)->min_rank_source->rank != (*it)->min_rank) && (*it) != e->source && !(*it)->isDone())
        {
            (*it)->min_rank = 0;
            (*it)->min_rank_source = nullptr;
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
            } else if (retval == nullptr || (retval->assignment == UNKNOWN && (*it)->assignment == ZERO) || (retval->assignment == (*it)->assignment && retval->rank < (*it)->rank)) {
                retval = *it;
                /*if((*it)->on_stack && e->source->on_stack && waiting != nullptr)
                {
                    bool single_loop = true;
                    int64_t n = waiting->size() - 1;
                    for(; n >= 0; ++n)
                    {
                        if((*waiting)[n].second.size() > 1)
                        {
                            single_loop = false;
                            break;
                        }
                        if((*waiting)[n].first == (*it)) break;
                    }
                    if(single_loop)
                    {
                        //std::cerr << (*it)->min_rank << " VS " << waiting->size() << std::endl;
                        //std::cerr << "ON STACK! Whooop " << n << std::endl;
                        hasCZero = true;
                        break;
                    }
                }*/
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
            auto* e = *cur;
            auto* c = e->source;
            if(c->isDone())
            {
                conf->dependency_set.erase_after(prev);
                cur = prev;
                ++cur;

                /*--e->refcnt;
                if (e->refcnt == 0)
                    graph->release(e);*/
                continue;
            }
            auto [und, assignment] = eval_edge(e);

            if(assignment == ONE)
            {
                set_assignment(c, ONE);
                waiting.push(c);
                /*--e->refcnt;
                if (e->refcnt == 0)
                    graph->release(e);*/
            }
            else if(assignment == UNKNOWN)
            {
                /*if(c->min_rank_source->)
                {
                    undef.emplace(c);
                    c->assignment = UNKNOWN;
                }
                else if(und->assignment == UNKNOWN)
                {
                    assert(!c->on_stack);
                    undef.emplace(c);
                }
                else
                {
                    assert(false); // do something
                }*/
            }
            else
            {
                bool all_zero = true;
                for(auto* e : c->successors)
                {
                    auto r = eval_edge(e);
                    all_zero &= r.second == CZERO;
                }
                if(all_zero) {
                    set_assignment(c, CZERO);
                    waiting.push(c);
                    /*--e->refcnt;
                    if (e->refcnt == 0)
                        graph->release(e);*/
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
                                mx = std::min(mx, t->min_rank);*/
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
                    // I suspect this *never* happens. The earlier check in
                    set_assignment(c, CZERO);
                    waiting.push(c);
                }
            }
            conf->dependency_set.erase_after(prev);
            cur = prev;
            ++cur;
        }
    }

    /*while(!undef.empty())
    {
        auto* conf = undef.top();
        std::cerr << "RESET " << conf->id << std::endl;
        conf->min_rank = 0;
        conf->min_rank_source = nullptr;
        conf->rank = 0;
        undef.pop();
        if(conf->isDone()) continue;
        assert(conf->assignment == UNKNOWN);
        if(conf->on_stack) {
            continue;
        }
        for(auto& e : conf->dependency_set)
        {
            auto* c = e->source;
            if(c->isDone() || c->on_stack)
            {
                continue;
            }
            --e->refcnt;
            if (e->refcnt == 0)
                graph->release(e);
            if(c->assignment != UNKNOWN)
            {
                c->assignment = UNKNOWN;
                undef.push(c);
            }
        }
        conf->dependency_set.clear();
    }*/
    return min;
}
