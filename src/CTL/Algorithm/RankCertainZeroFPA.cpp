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
    std::cerr << '(' << e->source->id;
    if (e->is_negated) {
        std::cerr << " -- ";
    } else {
        std::cerr << ", { ";
    }
    for (auto c: e->targets) {
        std::cerr << c->id << ' ';
    }
    std::cerr << (e->is_negated ? ")" : "})");
})
/*
std::map<std::string,std::string> correct;

std::pair<std::string,std::string> split(const std::string& line) {
    for(int64_t i = int64_t{line.size()}-1; i > 0; --i)
    {
        if(line[i] == ',')
            return std::make_pair(std::string(line.data(), i), std::string(line.data() + i, (line.size() - i)));
    }
    return {"",""};
}
 * */

void RankCertainZeroFPA::set_assignment(Configuration *c, Assignment a) {
    /*if(correct.empty())
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
    }*/
    c->assignment = a;
    //DEBUG_ONLY(std::cerr << "ASSIGN: [" << c->id << "]: " << to_string(static_cast<Assignment>(c->assignment)) << "\n";)
    /*if(c->isDone())
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
            else std::cerr << "¤ OK (" << c->id << ") = " << DependencyGraph::to_string((Assignment)c->assignment) << std::endl;
        }
        else std::cerr << "¤ NO MATCH (" << c->id << ") " << DependencyGraph::to_string((Assignment)c->assignment) << std::endl;
    }*/
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
    root->min_rank = root->rank = 1;
    std::stack<std::pair<DependencyGraph::Configuration*, std::vector<DependencyGraph::Edge*>>> waiting;
    waiting.emplace(root, graph->successors(root));
    root->on_stack = true;
    ++_exploredConfigurations;
    root->assignment = ZERO;
    //DEBUG_ONLY(std::cerr << "PUSH [" << root->id << "]" << std::endl;)
#ifdef DG_REFCOUNTING
    root->refc = 1;
#endif

    auto do_pop = [&waiting,this]() {
        auto& [conf, edges] = waiting.top();
        conf->on_stack = false;
        if(conf->isDone())
        {
            for(auto* e : edges)
            {
                e->status = DependencyGraph::EdgeStatus::NotWaiting;
                assert(e->refcnt == 1);
                e->refcnt = 0;
                graph->release(e);
            }
            conf->nsuccs = 0;
        }
        else
        {
            conf->nsuccs = 0;
            for(auto* e : edges)
            {
                if(e->refcnt == 1) // only in waiting
                {
                    for(auto& t : e->targets)
                    {
                        ++conf->nsuccs;
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
        waiting.pop();
    };

    while(!waiting.empty() && !root->isDone())
    {
        auto& [conf, edges] = waiting.top();
        if(conf->isDone()) {
            backprop(conf);
            //DEBUG_ONLY(std::cerr << "POP [" << conf->id << "] (assign(104) = " << to_string((Assignment)conf->assignment) << std::endl;)
            do_pop();
            continue;
        }
        Configuration* undecided = nullptr;
        Edge* undecided_edge = nullptr;
        bool all_czero = true;
        size_t j = 0;
        assert(conf->min_rank == conf->rank);
        for(size_t i = 0; i < edges.size(); ++i)
        {
            auto* e = edges[i];
            auto [ud, val] = eval_edge(e);
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
                assert(e->refcnt == 1);
                e->refcnt = 0;
                graph->release(e);
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
            backprop(conf);
            //graph->print(conf);
            //DEBUG_ONLY(std::cerr << "POP [" << conf->id << "] (assign(124) = " << to_string((Assignment)conf->assignment) << std::endl;)
            do_pop();
            continue;
        }

        if(undecided == nullptr || undecided->assignment != UNKNOWN) {
            //DEBUG_ONLY(std::cerr << "POP [" << conf->id << "] (undecided == nullptr)" << std::endl;)
            auto min_rank = conf->rank;
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
                    backprop(conf);
                    break;
                }

                size_t mx = 0;
                for(auto* t : e->targets)
                    if(!t->isDone())
                        mx = std::max(t->min_rank, mx);
                min_rank = std::min(min_rank, mx);
            }
            if(!conf->isDone())
            {
                if(min_rank >= conf->min_rank)
                {
                    set_assignment(conf, CZERO);
                    backprop(conf);
                }
                else
                {
                    conf->min_rank = min_rank;
                }
            }
            do_pop();
            continue;
        }
        else
        {
            //DEBUG_ONLY(std::cerr << "PUSH [" << undecided->id << "]" << std::endl;)
            undecided->min_rank = undecided->rank = conf->rank + 1;
            undecided->assignment = ZERO;
            waiting.emplace(undecided, graph->successors(undecided));
            undecided->on_stack = true;
            ++_exploredConfigurations;
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


std::pair<Configuration *, Assignment> Algorithm::RankCertainZeroFPA::eval_edge(DependencyGraph::Edge *e) {
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
        if ((*it)->assignment == ONE) {
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
            } else if (retval == nullptr || (retval->assignment == UNKNOWN && (*it)->assignment == ZERO)) {
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
    if(retval)
        return std::make_pair(retval, (Assignment)retval->assignment);
    else
    {
        assert(false);
        return std::make_pair(retval, ZERO);
    }
}

void Algorithm::RankCertainZeroFPA::backprop_edge(Edge* edge) {
    assert(false);
}

void Algorithm::RankCertainZeroFPA::backprop(Configuration* source) {
    assert(source->isDone());
    std::stack<Configuration*> waiting;
    std::stack<Configuration*> undef;
    waiting.emplace(source);

    while (!waiting.empty()) {
        auto* conf = waiting.top();
        assert(conf->isDone());

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

                --e->refcnt;
                if (e->refcnt == 0)
                    graph->release(e);
                continue;
            }
            auto [und, assignment] = eval_edge(e);

            if(assignment == ONE)
            {
                set_assignment(c, ONE);
                waiting.push(c);
                --e->refcnt;
                if (e->refcnt == 0)
                    graph->release(e);
            }
            else if(assignment == UNKNOWN)
            {
                if(c->assignment != UNKNOWN && !c->on_stack)
                {
                    undef.emplace(c);
                    c->assignment = UNKNOWN;
                }
            }
            else
            {
                assert(c->nsuccs > 0);
                --c->nsuccs;
                if(c->nsuccs == 0)
                {
                    set_assignment(c, CZERO);
                    waiting.push(c);
                    --e->refcnt;
                    if (e->refcnt == 0)
                        graph->release(e);
                }
            }
            conf->dependency_set.erase_after(prev);
            cur = prev;
            ++cur;
        }
    }

    while(!undef.empty())
    {
        auto* conf = undef.top();
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
    }
}
