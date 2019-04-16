#include "ReducingSuccessorGenerator.h"

#include "PQL/Contexts.h"

#include <assert.h>
#include <stack>
#include <unistd.h>

namespace PetriEngine {

    ReducingSuccessorGenerator::ReducingSuccessorGenerator(const PetriNet& net, bool is_game, bool is_safety) 
    : SuccessorGenerator(net, is_game, is_safety), _inhibpost(net._nplaces){
        _current = 0;
        _stub_enable = std::make_unique<uint8_t[]>(_net._ntransitions);
        _places_seen = std::make_unique<uint8_t[]>(_net.numberOfPlaces());
        _transitions = std::make_unique<strans_t[]>(_net.numberOfTransitions());
        reset();
        constructPrePost();
        constructDependency();
        computeFinite();
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

    void ReducingSuccessorGenerator::computeFinite() {
        std::stack<uint32_t> waiting;
        auto in_cnt = std::make_unique<uint32_t[]>(_net._nplaces);
        auto add_ppost_to_waiting = [&,this](auto p) {
            //std::cerr << "\t\tHANDLE " << _net.placeNames()[p] << std::endl;
            for(auto to = _places[p].post; to != _places[p+1].pre; ++to)
            {
                auto t = _arcs[to].index;
                if(!_transitions[t].finite)
                {
                    // we only care about env transitions
                    if(!_is_game || _net.ownedBy(t, _is_safety ? PetriNet::CTRL : PetriNet::ENV))
                    {
                        //std::cerr << "\t\t\tPUSH " << _net.transitionNames()[t] << " TO " << to << std::endl;
                        waiting.push(t);
                    }                    
                    _transitions[t].finite = true;
                }
                //if(_transitions[t].finite)
                //    std::cerr << "NOW FIN " << to << std::endl;
            }            
        };
        
        // bootstrap
        for(uint32_t p = 0; p < _net._nplaces; ++p)
        {
            in_cnt[p] = 0;
            //std::cerr << "CHECKING " << _net._placenames[p] << std::endl;
            for(auto ti = _places[p].pre; ti != _places[p].post; ++ti)
            {
                auto t = _arcs[ti].index;
                //std::cerr << "\t" << _net._transitionnames[t] << " TI " << ti << std::endl;
                if(_arcs[ti].direction >= 0 && // can maybe be refined if we set self-loops to inf?
                    (!_is_game || _net.ownedBy(t, _is_safety ? PetriNet::CTRL : PetriNet::ENV)))
                {
                    ++in_cnt[p];
                    //std::cerr << "\tP " << _net.placeNames()[p] << " " << _net.transitionNames()[t] << " :: " << in_cnt[p] << std::endl;

                }
                else
                {
                    //std::cerr << "\tSK " << _net.placeNames()[p] << std::endl;
                }
            }
        }
        
        // build waiting
        for(uint32_t p = 0; p < _net._nplaces; ++p)
            if(in_cnt[p] == 0)
                add_ppost_to_waiting(p);
        
        while(!waiting.empty())
        {
            auto t = waiting.top();
            //std::cerr << "POP " << t << std::endl;
            waiting.pop();
            //std::cerr << "POP " << _net.transitionNames()[t] << std::endl;
            for(auto it = _net.postset(t); it.first != it.second; ++it.first)
            {
                if(it.first->direction >= 0 && !it.first->inhibitor)
                {
                    //std::cerr << "B " << _net.placeNames()[it.first->place] << " " << _net.transitionNames()[t] << std::endl;
                    //std::cerr << "\t\t" << _net.placeNames()[it.first->place] << " :: " << in_cnt[it.first->place] << std::endl;
                    assert(in_cnt[it.first->place] > 0);
                    --in_cnt[it.first->place];
                    if(in_cnt[it.first->place] == 0)
                        add_ppost_to_waiting(it.first->place);
                }                
            }
        }
    }


    void ReducingSuccessorGenerator::computeSCC(uint32_t v, uint32_t& index, tarjan_t* data) {
        // tarjans algorithm : https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
        // TODO: Make this iterative (we could exceed the stacksize here)
        if(!_is_game && !_is_safety) return;
        auto& vd = data[v];
        vd.index = index;
        vd.lowlink = index;
        ++index;
        vd.on_stack = true;
        // we should make this iterative and not recursive
        for(auto tp = _places[v].post; tp < _places[v+1].pre; ++tp)
        {
            auto t = _arcs[tp].index;
            if(_is_game)
            {
                if(_is_safety && _net.ownedBy(t, PetriNet::ENV))
                    continue;
                if(!_is_safety && _net.ownedBy(t, PetriNet::CTRL))
                    continue;
            }
            if(_transitions[t].finite)
            {
                //std::cerr << "FINITE " << _net.transitionNames()[t] << std::endl;
                continue;
            }
            //std::cerr << "OWNER " << _net._transitionnames[t] << " OWNER " << (int) _net.owner(t) << std::endl;
            //if(_is_game) 
            //    std::cerr << "GOME " << std::endl;
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
                    _places[v].cycle = true;
              //      std::cerr << _net._placenames[v] << " IS CYCLE " << std::endl;
                    continue;
                }
                if(it->place != v && it->direction > 0)
                {   // only if we increment something in the post
                    assert(it->tokens > 0);
                    auto& wd = data[it->place];
                    if(wd.index == 0)
                    {
                //        std::cerr << "COMP SCC " << v << " : " << it->place << std::endl;
                //        std::cerr << vd.lowlink << " ID " << index << std::endl;
                        computeSCC(it->place, index, data);
                        vd.lowlink = std::min(vd.lowlink, wd.lowlink);
                        if(wd.lowlink <= vd.lowlink)
                            _places[v].cycle = true;
                //        std::cerr << "WD " << wd.lowlink << std::endl;
                    }
                    else if(wd.on_stack)
                    {
                        // an SCC
                //        std::cerr << "ONSTACK " << v << " : " << it->place << std::endl;
                        if(wd.index <= vd.index)
                            _places[v].cycle = true;
                        vd.lowlink = std::min(vd.lowlink, wd.index);
                    }
                }
            }        
        }
        vd.on_stack = false;
    }

    void ReducingSuccessorGenerator::computeSafe() {
        if(!_is_game) return;
        for(uint32_t t = 0; t < _net._ntransitions; ++t)
        {
            if(_net.ownedBy(t, PetriNet::CTRL))
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
                        if(_arcs[it].direction < 0)
                        {
                            auto id = _arcs[it].index;
                            if(_net.ownedBy(id, PetriNet::ENV))
                                continue;
                            // has to be consuming
                            _transitions[id].safe = false;
                            //std::cerr << "UNSAFE " << _net._transitionnames[id] << std::endl;
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
                        if(_arcs[it].direction > 0)
                        {
                            auto id = _arcs[it].index;
                            if(_net.ownedBy(id, PetriNet::ENV))
                                continue;
                            _transitions[id].safe = false;
                            //std::cerr << "UNSAFE " << _net._transitionnames[id] << std::endl;
                            break;
                        }
                    }
                }
                if(!_transitions[t].safe)
                {
                    for(auto pp = _net.postset(t); pp.first != pp.second; ++pp.first)
                    {
                        //std::cerr << "UNSAFE " << _net.placeNames()[pp.first->place] << std::endl;
                        _places[pp.first->place].safe = false;
                    }
                    break;
                }
            }            
        }        
    }
    
    void ReducingSuccessorGenerator::computeStaticCycles() {
        // standard DFS cycle detection
        auto data = std::make_unique<tarjan_t[]>(_net._nplaces);
        uint32_t index = 1;
        for(size_t p = 0; p < _net._nplaces; ++p)
            if(data[p].index == 0)
                computeSCC(p, index, data.get());
        
        /*std::cerr << "CYCLES PRE FIXPOINT" << std::endl;
        for(size_t p = 0; p < _net._nplaces; ++p)
        {
            if(_places[p].cycle)
                std::cerr << _net._placenames[p] << ", ";
        }
        std::cerr << std::endl;*/
        
        std::stack<uint32_t> stack;
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
                auto t = _arcs[it].index;
                if(_transitions[t].finite) 
                    continue;
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
        /*std::cerr << "CYCLES POST FIXPOINT" << std::endl;
        for(size_t p = 0; p < _net._nplaces; ++p)
        {
            if(_places[p].cycle)
                std::cerr << _net._placenames[p] << ", ";
        }
        std::cerr << std::endl;*/
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
                tmp_places[_net._invariants[finv].place].first.emplace_back(t, _net._invariants[finv].direction);
            }
        }

        // flatten
        size_t ntrans = 0;
        for (auto& p : tmp_places) {
            ntrans += p.first.size() + p.second.size();
        }
        _arcs = std::make_unique<trans_t[]>(ntrans);

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
                _arcs[tn + _places[p].pre] = pre[tn];
            }

            for (size_t tn = 0; tn < post.size(); ++tn) {
                _arcs[tn + _places[p].post] = post[tn];
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

            size_t dep = 0;
            for (; finv < linv; finv++) {
                const Invariant& inv = _net._invariants[finv];
                uint32_t p = inv.place;
                uint32_t ntrans = (_places[p + 1].pre - _places[p].post);
                dep += ntrans;
            }
            _transitions[t].dependency = dep;
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
            auto& tr = _arcs[t];
            if(tr.direction > 0)
            {
                addToStub(tr.index);
            }
        }
        if(make_closure) closure();            
    }
    
    void ReducingSuccessorGenerator::postsetOf(uint32_t place, bool make_closure) {       
        if(seenPost(place)) return;
        _places_seen[place] |= POSTSET;
        for (uint32_t t = _places[place].post; t < _places[place + 1].pre; t++) {
            auto tr = _arcs[t];
            if(tr.direction < 0)
            {
                addToStub(tr.index);
            }
        }
        if(make_closure) closure();
    }
    
    void ReducingSuccessorGenerator::addToStub(uint32_t t)
    {
        if((_stub_enable[t] & STUBBORN) == 0)
        {
            _stub_enable[t] |= STUBBORN;
            _unprocessed.push_back(t);
            //std::cerr << "\t\ts " << _net.transitionNames()[t] << std::endl;
            if(_is_game)
            {
                if( (_op_cand == std::numeric_limits<uint32_t>::max() || 
                        _transitions[t].dependency < _transitions[_op_cand].dependency ||
                        (_transitions[t].dependency == _transitions[_op_cand].dependency && t < _op_cand)) &&
                   ((_stub_enable[t] & ENABLED) == ENABLED) &&
                   (    
                        (_is_safety && !_is_game) || 
                        (_is_game && _net.ownedBy(t, _is_safety ? PetriNet::CTRL : PetriNet::ENV))
                    )
                )
                {
                    _op_cand = t;
                }
                if((_stub_enable[t] & ENABLED) == ENABLED)
                {
                    _added_enabled = true;
                    if(!_transitions[t].safe)
                        _added_unsafe = true;
                    //std::cerr << "\tUA " << _net.transitionNames()[t] << std::endl;
                }
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
        _added_enabled = false;
        reset();
        constructEnabled();

        if(_ordering.size() == 0) return;
        if(_ordering.size() == 1)
        {
            _skip = true;
            return;
        }

        for (auto &q : _queries) {
            q->evalAndSet(PQL::EvaluationContext(_parent, &_net, _is_game));
            q->findInteresting(*this, _is_safety);
            if(_added_unsafe) { _skip = true; return; }
        }
        closure();
        
        if(!_added_enabled)
            return;
        
        if(_players_enabled == PetriNet::ANY)
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
            {
                for(auto t : _ctrl_trans)
                    addToStub(t);
            }
            else
            {
                for(auto t : _env_trans)
                    addToStub(t);
            }
        }
        if(_added_unsafe) { _skip = true; return; }
        closure();
        if(_added_unsafe) { _skip = true; return; }
        
        if(( _is_game && _is_safety == (_players_enabled == PetriNet::CTRL)) ||
           (!_is_game && _is_safety))
        {
            if(_op_cand == std::numeric_limits<uint32_t>::max())
            {
                //std::cerr << "OPCAND WAS NOT SET" << std::endl;
                _op_cand = leastDependentEnabled();
            }
            if(_op_cand != std::numeric_limits<uint32_t>::max())
            {
                postPresetOf(_op_cand, true);
            }
        }
        /*std::cerr << "POST OPCAND " << (_added_unsafe ? "UNSAFE" : "") << std::endl;
        for(size_t t = 0; t < _net._ntransitions; ++t)
            if((_stub_enable[t] & (ENABLED)) != 0 && (_stub_enable[t] & STUBBORN) == 0)
                std::cerr << _net._transitionnames[t] << ",";
        std::cerr << std::endl;*/
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
                uint32_t t = _arcs[it].index;
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
                            addToStub(_arcs[t].index);
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
                uint32_t dep = std::numeric_limits<uint32_t>::max();
                // Lets try to see if we havent already added sufficient pre/post 
                // for this transition.
                // TODO: We can skip the unsafe guys here!
                for (; finv < linv; ++finv) {
                    const Invariant& inv = _net._invariants[finv];
                    bool change = true;
                    uint32_t d2 = std::numeric_limits<uint32_t>::max();
                    if(cand != std::numeric_limits<uint32_t>::max())
                    {
                        change = false;
                        if(_places[inv.place].safe && !_places[cand].safe)
                        {
                            change = true;
                        }
                        else if(_places[inv.place].safe == _places[cand].safe)
                        {
                            if(inv.inhibitor)
                                d2 = _places[inv.place+1].pre - _places[inv.place].post;
                            else
                                d2 = _places[inv.place].post - _places[inv.place].pre;
                            if(d2 <= dep)
                            {
                                if(d2 != dep || inv.place < cand)
                                    change = true;
                            }
                        }
                    }
                    if (_parent[inv.place] < inv.tokens && !inv.inhibitor) {
                        ok = seenPre(inv.place);
                        if(change)
                        {
                            inhib = false;
                            cand = inv.place;
                            dep = d2;
                        }
                    } else if (_parent[inv.place] >= inv.tokens && inv.inhibitor) {
                        ok = seenPost(inv.place);
                        if(change){
                            inhib = true;
                            cand = inv.place;
                            dep = d2;
                        }
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
                //std::cerr << "ns " << (int) player << " " << _net._transitionnames[_current] << std::endl;
                if(_is_game && !_net.ownedBy(_current, player))
                {
                    //std::cerr << "skip, other" << std::endl;
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
        _unprocessed.clear();
        _remaining.clear();
    }
}
