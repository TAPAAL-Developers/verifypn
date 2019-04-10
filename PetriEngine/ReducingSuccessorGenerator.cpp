#include "ReducingSuccessorGenerator.h"

#include "PQL/Contexts.h"

#include <assert.h>
#include <stack>
#include <unistd.h>

namespace PetriEngine {

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net, bool is_game) : SuccessorGenerator(net, is_game), _inhibpost(net._nplaces){
        _current = 0;
        _stub_enable = std::make_unique<uint8_t[]>(net._ntransitions);
        _dependency = std::make_unique<uint32_t[]>(net._ntransitions);
        _places_seen = std::make_unique<uint8_t[]>(_net.numberOfPlaces());
        _cycle_places = std::make_unique<bool[]>(_net.numberOfPlaces());
        reset();
        constructPrePost();
        constructDependency();
        checkForInhibitor();
        computeCycles();
    }

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries, bool is_game) : ReducingSuccessorGenerator(net, is_game) {
        _queries.reserve(queries.size());
        for(auto& q : queries)
            _queries.push_back(q.get());
    }

    void ReducingSuccessorGenerator::computeSCC(uint32_t v, uint32_t& index, tarjan_t* data, std::stack<uint32_t>& stack) {
        // tarjans algorithm : https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
        // TODO: Make this iterative (we could exceed the stacksize here)
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
            auto post = _net.postset(t);
            auto it = post.first;
            for(; it != post.second; ++it)
            {
                // only if we do not decrement (otherwise it would be finite)
                if(it->inhibitor) continue;
                if(it->place == v && it->direction >= 0)
                {
                    // selfloop, no need to continue SCC computation here
                    assert(it->tokens > 0);
                    _cycle_places[it->place] = true;
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
                _cycle_places[w] = true;
            } while(w != v);
        }
    }

    
    void ReducingSuccessorGenerator::computeCycles() {
        // standard DFS cycle detection
        auto data = std::make_unique<tarjan_t[]>(_net._nplaces);
        std::stack<uint32_t> stack;
        uint32_t index = 1;
        for(size_t p = 0; p < _net._nplaces; ++p)
            if(data[p].index == 0)
                computeSCC(p, index, data.get(), stack);
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
        memset(_dependency.get(), 0, sizeof(uint32_t) * _net._ntransitions);

        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            uint32_t finv = _net._transitions[t].inputs;
            uint32_t linv = _net._transitions[t].outputs;

            for (; finv < linv; finv++) {
                const Invariant& inv = _net._invariants[finv];
                uint32_t p = inv.place;
                uint32_t ntrans = (_places[p + 1].pre - _places[p].post);

                for (uint32_t tIndex = 0; tIndex < ntrans; tIndex++) {
                    _dependency[t]++;
                }
            }
        }
    }

    void ReducingSuccessorGenerator::constructEnabled() {
        for (uint32_t p = 0; p < _net._nplaces; ++p) {
            // orphans are currently under "place 0" as a special case
            if (p == 0 || _parent[p] > 0) { 
                uint32_t t = _net._placeToPtrs[p];
                uint32_t last = _net._placeToPtrs[p + 1];

                for (; t != last; ++t) {
                    if (!checkPreset(t)) continue;
                    _stub_enable[t] |= ENABLED;
                    _ordering.push_back(t);
                }
            }
        }
    }

    bool ReducingSuccessorGenerator::seenPre(uint32_t place) const
    {
        return (_places_seen[place] & 1) != 0;
    }
    
    bool ReducingSuccessorGenerator::seenPost(uint32_t place) const
    {
        return (_places_seen[place] & 2) != 0;
    }
    
    void ReducingSuccessorGenerator::presetOf(uint32_t place, bool make_closure) {
        if((_places_seen[place] & 1) != 0) return;
        _places_seen[place] = _places_seen[place] | 1;
        for (uint32_t t = _places[place].pre; t < _places[place].post; t++)
        {
            auto& tr = _transitions[t];
            addToStub(tr.index);
        }
        if(make_closure) closure();            
    }
    
    void ReducingSuccessorGenerator::postsetOf(uint32_t place, bool make_closure) {       
        if((_places_seen[place] & 2) != 0) return;
        _places_seen[place] = _places_seen[place] | 2;
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
        }
    }
    
    void ReducingSuccessorGenerator::inhibitorPostsetOf(uint32_t place){
        if((_places_seen[place] & 4) != 0) return;
        _places_seen[place] = _places_seen[place] | 4;
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
    

    void ReducingSuccessorGenerator::prepare(const MarkVal* state, PetriNet::player_t player) {
        _parent = state;
        _player = player;
        reset();
        constructEnabled();
        if(_ordering.size() == 0) return;
        if(_ordering.size() == 1)
        {
            _stub_enable[_ordering.front()] |= STUBBORN;
            return;
        }
        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext(_parent, &_net));
            q->findInteresting(*this, false);
        }
        
        closure();
    }
    
    void ReducingSuccessorGenerator::closure()
    {
        while (!_unprocessed.empty()) {
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
                        ok = (_places_seen[inv.place] & 1) != 0;
                        cand = inv.place;
                    } else if (_parent[inv.place] >= inv.tokens && inv.inhibitor) {
                        inhib = true;
                        ok = (_places_seen[inv.place] & 2) != 0;
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

    bool ReducingSuccessorGenerator::next(MarkVal* write) {
        while (!_ordering.empty()) {
            _current = _ordering.front();
            _ordering.pop_front();
            if ((_stub_enable[_current] & STUBBORN) == STUBBORN) {
                if(_is_game && !_net.ownedBy(_current, _player))
                {
                    _remaining.push_back(_current);
                    continue;
                }
                assert((_stub_enable[_current] & ENABLED) == ENABLED);
                memcpy(write, _parent, _net._nplaces*sizeof(MarkVal));
                consumePreset(write, _current);
                producePostset(write, _current);
                return true;
            }
        }
        return false;
    }
    
    uint32_t ReducingSuccessorGenerator::leastDependentEnabled() {
        uint32_t tLeast = -1;
        bool foundLeast = false;
        for (uint32_t t = 0; t < _net._ntransitions; t++) {
            if ((_stub_enable[t] & ENABLED) == ENABLED) {
                if (!foundLeast) {
                    tLeast = t;
                    foundLeast = true;
                } else {
                    if (_dependency[t] < _dependency[tLeast]) {
                        tLeast = t;
                    }
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
