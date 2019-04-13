#include "ReducingSuccessorGenerator.h"

#include "PQL/Contexts.h"

#include <assert.h>
#include <stack>
#include <unistd.h>

namespace PetriEngine {

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net, bool is_game, bool is_safety) 
    : SuccessorGenerator(net, is_game, is_safety), _inhibpost(net._nplaces){
        _current = 0;
        _stub_enable = std::make_unique<uint8_t[]>(net._ntransitions);
        _places_seen = std::make_unique<uint8_t[]>(_net.numberOfPlaces());
        reset();
        constructPrePost();
        constructDependency();
        checkForInhibitor();
        computeStaticCycles();
        computeSafetyOrphan();
        computeSafe();
        for(uint32_t t = 0; t < _net.numberOfTransitions(); ++t)
        {
            if(_net.ownedBy(t, PetriNet::CTRL))
                _ctrl_trans.push_back(t);
            else
                _env_trans.push_back(t);
        }
    }

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries, bool is_game, bool is_safety) 
    : ReducingSuccessorGenerator(net, is_game, is_safety) {
        _queries.reserve(queries.size());
        for(auto& q : queries)
            _queries.push_back(q.get());
    }

    void ReducingSuccessorGenerator::computeSCC(uint32_t v, uint32_t& index, tarjan_t* data, std::stack<uint32_t>& stack) {
        // tarjans algorithm : https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
        // TODO: Make this iterative (we could exceed the stacksize here)
        if(!_is_game && !_is_safety) return;
        auto& vd = data[v];
        vd.index = index;
        vd.lowlink = index;
        ++index;
        stack.push(v);
        vd.on_stack = true;
        // we should make this iterative and not recursive
        for(auto tp = _places[v].post; tp < _places[v+1].pre; ++tp)
        {
            auto t = _transitions[tp].index;
            if(_is_game)
            {
                if(_is_safety && _net.ownedBy(t, PetriNet::ENV))
                    continue;
                if(!_is_safety && _net.ownedBy(t, PetriNet::CTRL))
                    continue;
            }
            auto post = _net.postset(t);
            auto it = post.first;
            for(; it != post.second; ++it)
            {
                // only if we do not decrement (otherwise it would be finite)
                if(it->inhibitor) continue;
                if(it->place == v && it->direction >= 0)
                {
                    // selfloop, no need to continue SCC computation here
                    // we could try to analyze the cycle-types (inc/dec/etc)
                    assert(it->tokens > 0);
                    _places[it->place].cycle = true;
                    continue;
                }
                if(it->place != v && it->direction > 0)
                {   // only if we increment something in the post
                    assert(it->tokens > 0);
                    auto& wd = data[it->place];
                    if(wd.index == 0)
                    {
                        computeSCC(it->place, index, data, stack);
                        vd.lowlink = std::min(vd.lowlink, wd.lowlink);
                    }
                    else if(wd.on_stack)
                    {
                        // an SCC
                        vd.lowlink = std::min(vd.lowlink, wd.index);
                    }
                }
            }        
        }
        if(vd.lowlink == vd.index)
        {
            // everything in between is an SCC
            uint32_t w;
            do {
                w = stack.top();
                stack.pop();
                data[w].on_stack = false;
                _places[w].cycle = true;
            } while(w != v);
        }
    }

    void ReducingSuccessorGenerator::computeSafe() {
        if(!_is_game) return;
        for(uint32_t t = 0; t < _net._ntransitions; ++t)
        {
            if(!_net.ownedBy(t, PetriNet::ENV))
                continue;
            // check if the (t+)* contains an unctrl or 
            // (t-)* has unctrl with inhib
            auto pre = _net.preset(t);
            for(; pre.first != pre.second; ++pre.first)
            {
                // check ways we can enable this transition by ctrl
                if(pre.first->inhibitor)
                {
                    auto it = _places[pre.first->place].post;
                    auto end = _places[pre.first->place+1].pre;
                    for(; it != end; ++it)
                    {
                        if(_transitions[it].direction < 0)
                        {
                            auto id = _transitions[it].index;
                            if(_net.ownedBy(id, PetriNet::ENV))
                                continue;
                            // has to be consuming
                            _transitions[id].safe = false;
                            std::cerr << "UNSAFE " << _net._transitionnames[id] << std::endl;
                            break;
                        }
                    }
                }
                else
                {
                    auto it = _places[pre.first->place].pre;
                    auto end = _places[pre.first->place].post;
                    for(; it != end; ++it)
                    {
                        if(_transitions[it].direction > 0)
                        {
                            auto id = _transitions[it].index;
                            if(_net.ownedBy(id, PetriNet::ENV))
                                continue;
                            _transitions[id].safe = false;
                            std::cerr << "UNSAFE " << _net._transitionnames[id] << std::endl;
                            break;
                        }
                    }
                }
                if(!_transitions[t].safe)
                    break;
            }            
        }        
    }
    
    void ReducingSuccessorGenerator::computeStaticCycles() {
        // standard DFS cycle detection
        auto data = std::make_unique<tarjan_t[]>(_net._nplaces);
        std::stack<uint32_t> stack;
        uint32_t index = 1;
        for(size_t p = 0; p < _net._nplaces; ++p)
            if(data[p].index == 0)
                computeSCC(p, index, data.get(), stack);
        
        assert(stack.empty());
        // fixpoint with pre-sets
        for(uint32_t p = 0; p < _net._nplaces; ++p)
            if(_places[p].cycle)
                stack.push(p);
        while(!stack.empty())
        {
            auto p = stack.top();
            stack.pop();
            place_t& pl = _places[p];
            for(auto it = pl.pre; it != pl.post; ++it)
            {
                auto t = _transitions[it].index;
                if(_is_game)
                {
                    if(_is_safety && _net.ownedBy(t, PetriNet::ENV))
                        continue;
                    if(!_is_safety && _net.ownedBy(t, PetriNet::CTRL))
                        continue;
                }
                bool ok = true;
                // we can be more restrictive here.
                // only if we can feed an infinite number of tokens, we have to
                // consider it as part of a potential cycle. 
                for(auto pre = _net.preset(t); pre.first != pre.second; ++pre.first)
                {
                    if(pre.first->inhibitor) continue;
                    if(!_places[pre.first->place].cycle)
                    {
                        ok = false;
                        break;
                    }
                }
                if(ok)
                {
                    for(auto post = _net.postset(t); post.first != post.second; ++post.first)
                    {
                        if(!_places[post.first->place].cycle && post.first->direction > 0)
                        {
                            _places[post.first->place].cycle = true;
                            stack.push(post.first->place);
                        }
                    }
                }
            }
        }
    }

    void ReducingSuccessorGenerator::computeSafetyOrphan() {
        if(!_is_game && !_is_safety) return;
        for(size_t t = 0; t < _net._ntransitions; ++t)
        {
            if(_is_game)
            {
                // only care about orphans for the "winning by loop"-player
                if(_is_safety == _net.ownedBy(t, PetriNet::ENV))
                    continue;
            }
            auto pre = _net.preset(t);
            bool orphan = true;
            for(; pre.first != pre.second; ++pre.first)
            {
                if(pre.first->inhibitor)
                    continue;
                orphan = false;
                break;
            }
            if(orphan)
                _safety_orphans.push_back(orphan);
        }
    }

    
    void ReducingSuccessorGenerator::checkForInhibitor(){
        _netContainsInhibitorArcs = false;
        for (uint32_t t = 0; t < _net._ntransitions; ++t) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; ++finv) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _netContainsInhibitorArcs=true;
                    return;
                }
            }
        }
    }

    void ReducingSuccessorGenerator::constructPrePost() {
        std::vector<std::pair<std::vector<trans_t>, std::vector < trans_t>>> tmp_places(_net._nplaces);
                
        for (uint32_t t = 0; t < _net._ntransitions; ++t) {
            const TransPtr& ptr = _net._transitions[t];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            for (; finv < linv; finv++) { // Post set of places
                if (_net._invariants[finv].inhibitor) {
                    _inhibpost[_net._invariants[finv].place].push_back(t);
                    _netContainsInhibitorArcs = true;
                } else {
                    tmp_places[_net._invariants[finv].place].second.emplace_back(t, _net._invariants[finv].direction);
                }
            }

            finv = linv;
            linv = _net._transitions[t + 1].inputs;
            for (; finv < linv; finv++) { // Pre set of places
                if(_net._invariants[finv].direction > 0)
                    tmp_places[_net._invariants[finv].place].first.emplace_back(t, _net._invariants[finv].direction);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (auto& p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _transitions = std::make_unique<trans_t[]>(ntrans);

        _places = std::make_unique<place_t[]>(_net._nplaces + 1);
        uint32_t offset = 0;
        uint32_t p = 0;
        for (; p < _net._nplaces; ++p) {
            auto& pre = tmp_places[p].first;
            auto& post = tmp_places[p].second;

            // keep things nice for caches
            std::sort(pre.begin(), pre.end());
            std::sort(post.begin(), post.end());

            _places[p].pre = offset;
            offset += pre.size();
            _places[p].post = offset;
            offset += post.size();
            for (size_t tn = 0; tn < pre.size(); ++tn) {
                _transitions[tn + _places[p].pre] = pre[tn];
            }

            for (size_t tn = 0; tn < post.size(); ++tn) {
                _transitions[tn + _places[p].post] = post[tn];
            }

        }
        assert(offset == ntrans);
        _places[p].pre = offset;
        _places[p].post = offset;
    }

    void ReducingSuccessorGenerator::constructDependency() {

        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            uint32_t finv = _net._transitions[t].inputs;
            uint32_t linv = _net._transitions[t].outputs;

            for (; finv < linv; finv++) {
                const Invariant& inv = _net._invariants[finv];
                uint32_t p = inv.place;
                uint32_t ntrans = (_places[p + 1].pre - _places[p].post);
                _transitions[t].dependency += ntrans;
            }
        }
    }

    void ReducingSuccessorGenerator::constructEnabled() {
        _players_enabled = PetriNet::NONE;
        for (uint32_t p = 0; p < _net._nplaces; ++p) {
            // orphans are currently under "place 0" as a special case
            if (p == 0 || _parent[p] > 0) { 
                uint32_t t = _net._placeToPtrs[p];
                uint32_t last = _net._placeToPtrs[p + 1];

                for (; t != last; ++t) {
                    if (!checkPreset(t)) continue;
                    _stub_enable[t] |= ENABLED;
                    if(_is_game)
                    {
                        auto owner = _net.owner(t);
                        _players_enabled |= owner;
                    }
                    _ordering.push_back(t);
                }
            }
        }
    }

    bool ReducingSuccessorGenerator::seenPre(uint32_t place) const
    {
        return (_places_seen[place] & PRESET) != 0;
    }
    
    bool ReducingSuccessorGenerator::seenPost(uint32_t place) const
    {
        return (_places_seen[place] & POSTSET) != 0;
    }

    bool ReducingSuccessorGenerator::seenInhib(uint32_t place) const
    {
        return (_places_seen[place] & INHIB) != 0;
    }
    
    void ReducingSuccessorGenerator::presetOf(uint32_t place, bool make_closure) {
        if(seenPre(place)) return;
        _places_seen[place] |= PRESET;
        for (uint32_t t = _places[place].pre; t < _places[place].post; t++)
        {
            auto& tr = _transitions[t];
            addToStub(tr.index);
        }
        if(make_closure) closure();            
    }
    
    void ReducingSuccessorGenerator::postsetOf(uint32_t place, bool make_closure) {       
        if(seenPost(place)) return;
        _places_seen[place] |= POSTSET;
        for (uint32_t t = _places[place].post; t < _places[place + 1].pre; t++) {
            auto tr = _transitions[t];
            if(tr.direction < 0)
                addToStub(tr.index);
        }
        if(make_closure) closure();
    }
    
    void ReducingSuccessorGenerator::addToStub(uint32_t t)
    {
        if((_stub_enable[t] & STUBBORN) == 0)
        {
            _stub_enable[t] |= STUBBORN;
            _unprocessed.push_back(t);
            if(_is_game)
            {
                if( (_op_cand == std::numeric_limits<uint32_t>::max() || _transitions[t].dependency < _transitions[_op_cand].dependency )&& 
                   ((_stub_enable[t] & ENABLED) == ENABLED) &&
                   (    
                        (_is_safety && !_is_game) || 
                        (_is_game && _net.ownedBy(t, _is_safety ? PetriNet::CTRL : PetriNet::ENV))
                    )
                )
                {
                    _op_cand = t;
                }
                _added_unsafe |= !_transitions[t].safe;
            }
        }
    }
    
    void ReducingSuccessorGenerator::inhibitorPostsetOf(uint32_t place){
        if(seenInhib(place)) return;
        _places_seen[place] |= INHIB;
        for(uint32_t& newstub : _inhibpost[place])
            addToStub(newstub);
    }
    
    void ReducingSuccessorGenerator::postPresetOf(uint32_t t, bool make_closure) {
        const TransPtr& ptr = _net._transitions[t];
        uint32_t finv = ptr.inputs;
        uint32_t linv = ptr.outputs;
        for (; finv < linv; finv++) { // pre-set of t
            if(_net._invariants[finv].inhibitor){ 
                presetOf(_net._invariants[finv].place, make_closure);
            } else {
                postsetOf(_net._invariants[finv].place, make_closure); 
            }
        }        
    }
    

    void ReducingSuccessorGenerator::prepare(const MarkVal* state) {
        _parent = state;
        _skip = false;
        _op_cand = std::numeric_limits<uint32_t>::max();
        _added_unsafe = false;
        reset();
        constructEnabled();

        if(_ordering.size() == 0) return;
        if(_ordering.size() == 1 || 
           _players_enabled == PetriNet::ANY)
        {
            _skip = true;
            return;
        }
        
        if(!_is_game) 
        {
            // we only need to preserve cycles in case of safety
            if(_is_safety)
                preserveCycles();
        }
        else
        {
            // preserve cycles for the player winning by cycles
            if(_is_safety == (_players_enabled == PetriNet::CTRL))
                preserveCycles();
            if(_added_unsafe) { _skip = true; return; }
            if(_players_enabled == PetriNet::ENV)
                for(auto t : _ctrl_trans) _unprocessed.push_back(t);
            else
                for(auto t : _env_trans) _unprocessed.push_back(t);
        }
        if(_added_unsafe) { _skip = true; return; }
        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext(_parent, &_net, _is_game));
            q->findInteresting(*this, false);
            if(_added_unsafe) { _skip = true; return; }
        }
                
        closure();
        if(_added_unsafe) { _skip = true; return; }
        if(( _is_game && _is_safety == (_players_enabled == PetriNet::CTRL)) ||
           (!_is_game && _is_safety))
        {
            if(_op_cand == std::numeric_limits<uint32_t>::max())
            {
                _op_cand = leastDependentEnabled();
            }
            if(_op_cand != std::numeric_limits<uint32_t>::max())
            {
                postPresetOf(_op_cand, true);
            }
        }
        if(_added_unsafe) { _skip = true; return; }
    }

    void ReducingSuccessorGenerator::preserveCycles() 
    {
        for(auto& t : _safety_orphans)
        {
            addToStub(t);
            if(_added_unsafe)
                return;
        }
        std::stack<uint32_t> waiting;
        for(size_t p = 0; p < _net._nplaces; ++p)
        {
            if(_parent[p] > 0)
            {
                waiting.push(p);
                _places_seen[p] |= MARKED;
            }
        }

        while(!waiting.empty())
        {
            auto p = waiting.top();
            waiting.pop();
            auto it = _places[p].pre;
            auto stop = _places[p].post;
            for(; it != stop; ++it)
            {
                // check if any transitions pre is in marked places
                uint32_t t = _transitions[it].index;
                if(_is_game && _net.ownedBy(t, (_is_safety ? PetriNet::ENV : PetriNet::CTRL)))
                    continue;
                if((_stub_enable[t] & (ADDED_POST | STUBBORN)) == 0)
                {
                    auto pre = _net.preset(t);
                    bool ok = true;
                    bool cycle = true;
                    // we can do cycle and fresh-check in one go.
                    for(;pre.first != pre.second; ++pre.first)
                    {
                        if(pre.first->inhibitor)
                            continue;
                        if((_places_seen[pre.first->place] & MARKED) == 0)
                        {
                            cycle = ok = false;
                            break;
                        }
                        if(!_places[pre.first->place].cycle)
                            cycle = false;
                    }
                    if(ok)
                    {
                        // add post
                        _stub_enable[t] |= ADDED_POST;
                        auto post = _net.postset(t);
                        for(;post.first != post.second; ++post.first)
                        {
                            if(((_places_seen[pre.first->place] & MARKED) == 0)) // post.first->direction > 0 <-- redundant
                            {
                                waiting.push(post.first->place);
                                _places_seen[pre.first->place] |= MARKED;
                            }
                        }
                    }
                    if(cycle)
                    {
                        addToStub(t);
                        if(_added_unsafe) return;
                    }
                }
            }
        }
    }
    
    void ReducingSuccessorGenerator::closure()
    {
        while (!_unprocessed.empty()) {
            if(_added_unsafe)
                return;
            uint32_t tr = _unprocessed.front();
            _unprocessed.pop_front();
            const TransPtr& ptr = _net._transitions[tr];
            uint32_t finv = ptr.inputs;
            uint32_t linv = ptr.outputs;
            if((_stub_enable[tr] & ENABLED) == ENABLED){
                for (; finv < linv; finv++) {
                    if(_net._invariants[finv].direction < 0)
                    {
                        auto place = _net._invariants[finv].place;
                        for (uint32_t t = _places[place].post; t < _places[place + 1].pre; t++) 
                            addToStub(_transitions[t].index);
                    }
                }
                if(_netContainsInhibitorArcs){
                    uint32_t next_finv = _net._transitions[tr+1].inputs;
                    for (; linv < next_finv; linv++)
                    {
                        if(_net._invariants[linv].direction > 0)
                            inhibitorPostsetOf(_net._invariants[linv].place);
                    }
                }
            } else {
                bool ok = false;
                bool inhib = false;
                uint32_t cand = std::numeric_limits<uint32_t>::max();
               
                // Lets try to see if we havent already added sufficient pre/post 
                // for this transition.
                for (; finv < linv; ++finv) {
                    const Invariant& inv = _net._invariants[finv];
                    if (_parent[inv.place] < inv.tokens && !inv.inhibitor) {
                        inhib = false;
                        ok = seenPre(inv.place);
                        cand = inv.place;
                    } else if (_parent[inv.place] >= inv.tokens && inv.inhibitor) {
                        inhib = true;
                        ok = seenPost(inv.place);
                        cand = inv.place;
                    }
                    if(ok) break;

                }
                
                // OK, we didnt have sufficient, we just pick whatever is left
                // in cand.
                assert(cand != std::numeric_limits<uint32_t>::max());
                if(!ok && cand != std::numeric_limits<uint32_t>::max())
                {
                    if(!inhib) presetOf(cand);
                    else       postsetOf(cand);
                }
            }
        }        
    }
    
    void ReducingSuccessorGenerator::fireCurrent(MarkVal* write) {
        memcpy(write, _parent, _net._nplaces*sizeof(MarkVal));
        consumePreset(write, _current);
        producePostset(write, _current);        
    }

    bool ReducingSuccessorGenerator::next(MarkVal* write, PetriNet::player_t player) {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if ((_stub_enable[_current] & STUBBORN) == STUBBORN || _skip) {
                if(_is_game && !_net.ownedBy(_current, player))
                {
                    _remaining.push_back(_current);
                    continue;
                }
                fireCurrent(write);
                return true;
            }
        }
        _remaining.swap(_ordering);
        return false;
    }
    
    uint32_t ReducingSuccessorGenerator::leastDependentEnabled() {
        uint32_t tLeast = std::numeric_limits<uint32_t>::max();
        uint32_t lval = std::numeric_limits<uint32_t>::max();
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            if ((_stub_enable[t] & ENABLED) == ENABLED) {
                auto dep = _transitions[t].dependency;
                if (dep < lval) {
                    tLeast = t;
                    lval = dep;
                }
            }
        }
        return tLeast;
    }

    void ReducingSuccessorGenerator::reset() {
        SuccessorGenerator::reset();
        memset(_stub_enable.get(), 0, sizeof(bool) * _net._ntransitions);
        memset(_places_seen.get(), 0, _net.numberOfPlaces());
        _ordering.clear();
        _remaining.clear();
    }
}
