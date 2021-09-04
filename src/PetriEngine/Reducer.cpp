/*
 * File:   Reducer.cpp
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#include "PetriEngine/Reducer.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriParse/PNMLParser.h"
#include <queue>
#include <set>
#include <algorithm>

namespace PetriEngine {

    Reducer::Reducer(PetriNetBuilder* p)
    : _builder(p) {
    }

    Reducer::~Reducer() {

    }

    void Reducer::print(QueryPlaceAnalysisContext& context) {
        std::cout   << "\nNET INFO:\n"
                    << "Number of places: " << _builder->number_of_places() << std::endl
                    << "Number of transitions: " << _builder->number_of_transitions()
                    << std::endl << std::endl;
        for (uint32_t t = 0; t < _builder->number_of_transitions(); t++) {
            std::cout << "Transition " << t << " :\n";
            if(_builder->_transitions[t]._skip)
            {
                std::cout << "\tSKIPPED" << std::endl;
            }
            for(auto& arc : _builder->_transitions[t]._pre)
            {
                if (arc._weight > 0)
                    std::cout   << "\tInput place " << arc._place
                                << " (" << get_place_name(arc._place) << ")"
                                << " with arc-weight " << arc._weight << std::endl;
            }
            for(auto& arc : _builder->_transitions[t]._post)
            {
                if (arc._weight > 0)
                    std::cout   << "\tOutput place " << arc._place
                                << " (" << get_place_name(arc._place) << ")"
                                << " with arc-weight " << arc._weight << std::endl;
            }
            std::cout << std::endl;
        }
        for (uint32_t i = 0; i < _builder->number_of_places(); i++) {
            std::cout <<    "Marking at place "<< i <<
                            " is: " << _builder->init_marking()[i] << std::endl;
        }
        for (uint32_t i = 0; i < _builder->number_of_places(); i++) {
            std::cout   << "Query count for place " << i
                        << " is: " << context.get_query_placeCount()[i] << std::endl;
        }
    }

    std::string Reducer::get_transition_name(uint32_t transition)
    {
        for(auto t : _builder->_transitionnames)
        {
            if(t.second == transition) return t.first;
        }
        assert(false);
        return "";
    }

    std::string Reducer::new_trans_name()
    {
        auto prefix = "CT";
        auto tmp = prefix + std::to_string(_tnameid);
        while(_builder->_transitionnames.count(tmp) >= 1)
        {
            ++_tnameid;
            tmp = prefix + std::to_string(_tnameid);
        }
        ++_tnameid;
        return tmp;
    }

    std::string Reducer::get_place_name(uint32_t place)
    {
        for(auto t : _builder->_placenames)
        {
            if(t.second == place) return t.first;
        }
        assert(false);
        return "";
    }

    Transition& Reducer::get_transition(uint32_t transition)
    {
        return _builder->_transitions[transition];
    }

    ArcIter Reducer::get_out_arc(Transition& trans, uint32_t place)
    {
        Arc a;
        a._place = place;
        auto ait = std::lower_bound(trans._post.begin(), trans._post.end(), a);
        if(ait != trans._post.end() && ait->_place == place)
        {
            return ait;
        }
        else
        {
            return trans._post.end();
        }
    }

    ArcIter Reducer::get_in_arc(uint32_t place, Transition& trans)
    {
        Arc a;
        a._place = place;
        auto ait = std::lower_bound(trans._pre.begin(), trans._pre.end(), a);
        if(ait != trans._pre.end() && ait->_place == place)
        {
            return ait;
        }
        else
        {
            return trans._pre.end();
        }
    }

    void Reducer::erase_transition(std::vector<uint32_t>& set, uint32_t el)
    {
        auto lb = std::lower_bound(set.begin(), set.end(), el);
        assert(lb != set.end());
        assert(*lb == el);
        set.erase(lb);
    }

    void Reducer::skip_transition(uint32_t t)
    {
        ++_removedTransitions;
        Transition& trans = get_transition(t);
        assert(!trans._skip);
        for(auto p : trans._post)
        {
            erase_transition(_builder->_places[p._place]._producers, t);
        }
        for(auto p : trans._pre)
        {
            erase_transition(_builder->_places[p._place]._consumers, t);
        }
        trans._post.clear();
        trans._pre.clear();
        trans._skip = true;
        assert(consistent());
        _skipped_trans.push_back(t);
    }

    void Reducer::skip_place(uint32_t place)
    {
        ++_removedPlaces;
        Place& pl = _builder->_places[place];
        assert(!pl._skip);
        pl._skip = true;
        for(auto& t : pl._consumers)
        {
            Transition& trans = get_transition(t);
            auto ait = get_in_arc(place, trans);
            if(ait != trans._pre.end() && ait->_place == place)
                trans._pre.erase(ait);
        }

        for(auto& t : pl._producers)
        {
            Transition& trans = get_transition(t);
            auto ait = get_out_arc(trans, place);
            if(ait != trans._post.end() && ait->_place == place)
                trans._post.erase(ait);
        }
        pl._consumers.clear();
        pl._producers.clear();
        assert(consistent());
    }


    bool Reducer::consistent()
    {
#ifndef NDEBUG
        size_t strans = 0;
        for(size_t i = 0; i < _builder->number_of_transitions(); ++i)
        {
            Transition& t = _builder->_transitions[i];
            if(t._skip) ++strans;
            assert(std::is_sorted(t._pre.begin(), t._pre.end()));
            assert(std::is_sorted(t._post.end(), t._post.end()));
            assert(!t._skip || (t._pre.size() == 0 && t._post.size() == 0));
            for(Arc& a : t._pre)
            {
                assert(a._weight > 0);
                Place& p = _builder->_places[a._place];
                assert(!p._skip);
                assert(std::find(p._consumers.begin(), p._consumers.end(), i) != p._consumers.end());
            }
            for(Arc& a : t._post)
            {
                assert(a._weight > 0);
                Place& p = _builder->_places[a._place];
                assert(!p._skip);
                assert(std::find(p._producers.begin(), p._producers.end(), i) != p._producers.end());
            }
        }

        assert(strans == _removedTransitions);

        size_t splaces = 0;
        for(size_t i = 0; i < _builder->number_of_places(); ++i)
        {
            Place& p = _builder->_places[i];
            if(p._skip) ++splaces;
            assert(std::is_sorted(p._consumers.begin(), p._consumers.end()));
            assert(std::is_sorted(p._producers.begin(), p._producers.end()));
            assert(!p._skip || (p._consumers.size() == 0 && p._producers.size() == 0));

            for(uint c : p._consumers)
            {
                Transition& t = _builder->_transitions[c];
                assert(!t._skip);
                auto a = get_in_arc(i, t);
                assert(a != t._pre.end());
                assert(a->_place == i);
            }

            for(uint prod : p._producers)
            {
                Transition& t = _builder->_transitions[prod];
                assert(!t._skip);
                auto a = get_out_arc(t, i);
                assert(a != t._post.end());
                assert(a->_place == i);
            }
        }
        assert(splaces == _removedPlaces);
#endif
        return true;
    }

    bool Reducer::rule_a(uint32_t* placeInQuery) {
        // Rule A  - find transition t that has exactly one place in pre and post and remove one of the places (and t)
        bool continueReductions = false;
        const size_t number_of_transitions = _builder->number_of_transitions();
        for (uint32_t t = 0; t < number_of_transitions; t++) {
            if(has_timed_out()) return false;
            Transition& trans = get_transition(t);

            // we have already removed
            if(trans._skip) continue;

            // A2. we have more/less than one arc in pre or post
            // checked first to avoid out-of-bounds when looking up indexes.
            if(trans._pre.size() != 1) continue;

            uint32_t pPre = trans._pre[0]._place;

            // A2. Check that pPre goes only to t
            if(_builder->_places[pPre]._consumers.size() != 1) continue;

            // A3. We have weight of more than one on input
            // and is empty on output (should not happen).
            auto w = trans._pre[0]._weight;
            bool ok = true;
            for(auto t : _builder->_places[pPre]._producers)
            {
                if((get_out_arc(_builder->_transitions[t], trans._pre[0]._place)->_weight % w) != 0)
                {
                    ok = false;
                    break;
                }
            }
            if(!ok)
                continue;

            // A4. Do inhibitor check, neither T, pPre or pPost can be involved with any inhibitor
            if(_builder->_places[pPre]._inhib|| trans._inhib) continue;

            // A5. dont mess with query!
            if(placeInQuery[pPre] > 0) continue;
            // check A1, A4 and A5 for post
            for(auto& pPost : trans._post)
            {
                if(_builder->_places[pPost._place]._inhib || pPre == pPost._place || placeInQuery[pPost._place] > 0)
                {
                    ok = false;
                    break;
                }
            }
            if(!ok) continue;

            continueReductions = true;
            _ruleA++;

            // here we need to remember when a token is created in pPre (some
            // transition with an output in P is fired), t is fired instantly!.
            if(_reconstruct_trace) {
                Place& pre = _builder->_places[pPre];
                std::string tname = get_transition_name(t);
                for(size_t pp : pre._producers)
                {
                    std::string prefire = get_transition_name(pp);
                    _postfire[prefire].push_back(tname);
                }
                _extraconsume[tname].emplace_back(get_place_name(pPre), w);
                for(size_t i = 0; i < _builder->init_marking()[pPre]; ++i)
                {
                    _initfire.push_back(tname);
                }
            }

            for(auto& pPost : trans._post)
            {
                // UA2. move the token for the initial marking, makes things simpler.
                _builder->_initial_marking[pPost._place] += ((_builder->_initial_marking[pPre]/w) * pPost._weight);
            }
            _builder->_initial_marking[pPre] = 0;

            // Remove transition t and the place that has no tokens in m0
            // UA1. remove transition
            auto toMove = trans._post;
            skip_transition(t);

            // UA2. update arcs
            for(auto& _t : _builder->_places[pPre]._producers)
            {
                assert(_t != t);
                // move output-arcs to post.
                Transition& src = get_transition(_t);
                auto source = *get_out_arc(src, pPre);
                for(auto& pPost : toMove)
                {
                    Arc a;
                    a._place = pPost._place;
                    a._weight = (source._weight/w) * pPost._weight;
                    assert(a._weight > 0);
                    a._inhib = false;
                    auto dest = std::lower_bound(src._post.begin(), src._post.end(), a);
                    if(dest == src._post.end() || dest->_place != pPost._place)
                    {
                        dest = src._post.insert(dest, a);
                        auto& prod = _builder->_places[pPost._place]._producers;
                        auto lb = std::lower_bound(prod.begin(), prod.end(), _t);
                        prod.insert(lb, _t);
                    }
                    else
                    {
                        dest->_weight += ((source._weight/w) * pPost._weight);
                    }
                    assert(dest->_weight > 0);
                }
            }
            // UA1. remove place
            skip_place(pPre);
        } // end of Rule A main for-loop
        return continueReductions;
    }

    bool Reducer::rule_b(uint32_t* placeInQuery, bool remove_deadlocks, bool remove_consumers) {

        // Rule B - find place p that has exactly one transition in pre and exactly one in post and remove the place
        bool continueReductions = false;
        const size_t number_of_places = _builder->number_of_places();
        for (uint32_t p = 0; p < number_of_places; p++) {
            if(has_timed_out()) return false;
            Place& place = _builder->_places[p];

            if(place._skip) continue;    // already removed
            // B5. dont mess up query
            if(placeInQuery[p] > 0)
                continue;

            // B2. Only one consumer/producer
            if( place._consumers.size() != 1 ||
                place._producers.size() < 1)
                continue; // no orphan removal

            auto tIn = place._consumers[0];

            // B1. producer is not consumer
            bool ok = true;
            for(auto& tOut : place._producers)
            {
                if (tOut == tIn)
                {
                    ok = false;
                    continue; // cannot remove this kind either
                }
            }
            if(!ok)
                continue;
            auto prod = place._producers;
            Transition& in = get_transition(tIn);
            for(auto tOut : prod)
            {
                Transition& out = get_transition(tOut);

                if(out._post.size() != 1 && in._pre.size() != 1)
                    continue; // at least one has to be singular for this to work

                if((!remove_deadlocks || !remove_consumers) && in._pre.size() != 1)
                    // the buffer can mean deadlocks and other interesting things
                    // also we can "hide" tokens, so we need to make sure not
                    // to remove consumers.
                    continue;

                if(_builder->init_marking()[p] > 0 && in._pre.size() != 1)
                    continue;

                auto inArc = get_in_arc(p, in);
                auto outArc = get_out_arc(out, p);

                // B3. Output is a multiple of input and nonzero.
                if(outArc->_weight < inArc->_weight)
                    continue;
                if((outArc->_weight % inArc->_weight) != 0)
                    continue;

                size_t multiplier = outArc->_weight / inArc->_weight;

                // B4. Do inhibitor check, neither In, out or place can be involved with any inhibitor
                if(place._inhib || in._inhib || out._inhib)
                    continue;

                // B6. also, none of the places in the post-set of consuming transition can be participating in inhibitors.
                // B7. nor can they appear in the query.
                {
                    bool post_ok = false;
                    for(const Arc& a : in._post)
                    {
                        post_ok |= _builder->_places[a._place]._inhib;
                        post_ok |= placeInQuery[a._place];
                        if(post_ok) break;
                    }
                    if(post_ok)
                        continue;
                }
                {
                    bool pre_ok = false;
                    for(const Arc& a : in._pre)
                    {
                        pre_ok |= _builder->_places[a._place]._inhib;
                        pre_ok |= placeInQuery[a._place];
                        if(pre_ok) break;
                    }
                    if(pre_ok)
                        continue;
                }

                bool ok = true;
                if(in._pre.size() > 1)
                    for(const Arc& arc : out._pre)
                        ok &= placeInQuery[arc._place] == 0;
                if(!ok)
                    continue;

                // B2.a Check that there is no other place than p that gives to tPost,
                // tPre can give to other places
                auto& arcs = in._pre.size() < out._post.size() ? in._pre : out._post;
                for (auto& arc : arcs) {
                    if (arc._weight > 0 && arc._place != p) {
                        ok = false;
                        break;
                    }
                }

                if (!ok)
                    continue;

                // UB2. we need to remember initial marking
                uint initm = _builder->init_marking()[p];
                initm /= inArc->_weight; // integer-devision is floor by default

                if(_reconstruct_trace)
                {
                    // remember reduction for recreation of trace
                    std::string toutname    = get_transition_name(tOut);
                    std::string tinname     = get_transition_name(tIn);
                    std::string pname       = get_place_name(p);
                    Arc& a = *get_in_arc(p, in);
                    _extraconsume[tinname].emplace_back(pname, a._weight);
                    for(size_t i = 0; i < multiplier; ++i)
                    {
                        _postfire[toutname].push_back(tinname);
                    }

                    for(size_t i = 0; initm > 0 && i < initm / inArc->_weight; ++i )
                    {
                        _initfire.push_back(tinname);
                    }
                }

                continueReductions = true;
                _ruleB++;
                 // UB1. Remove place p
                _builder->_initial_marking[p] = 0;
                // We need to remember that when tOut fires, tIn fires just after.
                // this should fix the trace

                // UB3. move arcs from t' to t
                for (auto& arc : in._post) { // remove tPost
                    auto _arc = get_out_arc(out, arc._place);
                    // UB2. Update initial marking
                    _builder->_initial_marking[arc._place] += initm*arc._weight;
                    if(_arc != out._post.end())
                    {
                        _arc->_weight += arc._weight*multiplier;
                    }
                    else
                    {
                        out._post.push_back(arc);
                        out._post.back()._weight *= multiplier;
                        _builder->_places[arc._place]._producers.push_back(tOut);

                        std::sort(out._post.begin(), out._post.end());
                        std::sort(_builder->_places[arc._place]._producers.begin(),
                                  _builder->_places[arc._place]._producers.end());
                    }
                }
                for (auto& arc : in._pre) { // remove tPost
                    if(arc._place == p)
                        continue;
                    auto _arc = get_in_arc(arc._place, out);
                    // UB2. Update initial marking
                    _builder->_initial_marking[arc._place] += initm*arc._weight;
                    if(_arc != out._pre.end())
                    {
                        _arc->_weight += arc._weight*multiplier;
                    }
                    else
                    {
                        out._pre.push_back(arc);
                        out._pre.back()._weight *= multiplier;
                        _builder->_places[arc._place]._consumers.push_back(tOut);

                        std::sort(out._pre.begin(), out._pre.end());
                        std::sort(_builder->_places[arc._place]._consumers.begin(),
                                  _builder->_places[arc._place]._consumers.end());
                    }
                }

                for(auto it = out._post.begin(); it != out._post.end(); ++it)
                {
                    if(it->_place == p)
                    {
                        out._post.erase(it);
                        break;
                    }
                }
                for(auto it = place._producers.begin(); it != place._producers.end(); ++it)
                {
                    if(*it == tOut)
                    {
                        place._producers.erase(it);
                        break;
                    }
                }
            }
            // UB1. remove transition
            if(place._producers.size() == 0)
            {
                skip_place(p);
                skip_transition(tIn);
            }
        } // end of Rule B main for-loop
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::rule_c(uint32_t* placeInQuery) {
        // Rule C - Places with same input and output-transitions which a modulo each other
        bool continueReductions = false;

        _pflags.resize(_builder->_places.size(), 0);
        std::fill(_pflags.begin(), _pflags.end(), 0);

        for(uint32_t touter = 0; touter < _builder->number_of_transitions(); ++touter)
        for(size_t outer = 0; outer < _builder->_transitions[touter]._post.size(); ++outer)
        {
            auto pouter = _builder->_transitions[touter]._post[outer]._place;
            if(_pflags[pouter] > 0) continue;
            _pflags[pouter] = 1;
            if(has_timed_out()) return false;
            if(_builder->_places[pouter]._skip) continue;

            // C4. No inhib
            if(_builder->_places[pouter]._inhib) continue;

            for (size_t inner = outer + 1; inner < _builder->_transitions[touter]._post.size(); ++inner)
            {
                auto pinner = _builder->_transitions[touter]._post[inner]._place;
                if(_builder->_places[pinner]._skip) continue;

                // C4. No inhib
                if(_builder->_places[pinner]._inhib) continue;

                for(size_t swp = 0; swp < 2; ++swp)
                {
                    if(has_timed_out()) return false;
                    if( _builder->_places[pinner]._skip ||
                        _builder->_places[pouter]._skip) break;

                    uint p1 = pouter;
                    uint p2 = pinner;

                    if(swp == 1) std::swap(p1, p2);

                    Place& place1 = _builder->_places[p1];

                    // C1. Not same place
                    if(p1 == p2) break;

                    // C5. Dont mess with query
                    if(placeInQuery[p2] > 0)
                        continue;

                    Place& place2 = _builder->_places[p2];

                    // C2, C3. Consumer and producer-sets must match
                    if(place1._consumers.size() < place2._consumers.size() ||
                       place1._producers.size() > place2._producers.size())
                        break;

                    long double mult = 1;

                    // C8. Consumers must match with weights
                    int ok = 0;
                    size_t j = 0;
                    for(size_t i = 0; i < place2._consumers.size(); ++i)
                    {
                        while(j < place1._consumers.size() && place1._consumers[j] < place2._consumers[i] ) ++j;
                        if(place1._consumers.size() <= j || place1._consumers[j] != place2._consumers[i])
                        {
                            ok = 2;
                            break;
                        }

                        Transition& trans = get_transition(place1._consumers[j]);
                        auto a1 = get_in_arc(p1, trans);
                        auto a2 = get_in_arc(p2, trans);
                        assert(a1 != trans._pre.end());
                        assert(a2 != trans._pre.end());
                        mult = std::max(mult, ((long double)a2->_weight) / ((long double)a1->_weight));
                    }

                    if(ok == 2) break;

                    // C6. We do not care about excess markings in p2.
                    if(mult != std::numeric_limits<long double>::max() &&
                            (((long double)_builder->_initial_marking[p1]) * mult) > ((long double)_builder->_initial_marking[p2]))
                    {
                        continue;
                    }


                    // C7. Producers must match with weights
                    j = 0;
                    for(size_t i = 0; i < place1._producers.size(); ++i)
                    {
                        while(j < place2._producers.size() && place2._producers[j] < place1._producers[i]) ++j;
                        if(j == place2._producers.size() || place1._producers[i] != place2._producers[j])
                        {
                            ok = 2;
                            break;
                        }

                        Transition& trans = get_transition(place1._producers[i]);
                        auto a1 = get_out_arc(trans, p1);
                        auto a2 = get_out_arc(trans, p2);
                        assert(a1 != trans._post.end());
                        assert(a2 != trans._post.end());

                        if(((long double)a1->_weight)*mult > ((long double)a2->_weight))
                        {
                            ok = 1;
                            break;
                        }
                    }

                    if(ok == 2) break;
                    else if(ok == 1) continue;

                    _builder->_initial_marking[p2] = 0;

                    if(_reconstruct_trace)
                    {
                        for(auto t : place2._consumers)
                        {
                            std::string tname = get_transition_name(t);
                            const ArcIter arc = get_in_arc(p2, get_transition(t));
                            _extraconsume[tname].emplace_back(get_place_name(p2), arc->_weight);
                        }
                    }

                    continueReductions = true;
                    _ruleC++;
                    // UC1. Remove p2
                    skip_place(p2);
                    _pflags[pouter] = 0;
                    break;
                }
            }
        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::rule_d(uint32_t* placeInQuery) {
        // Rule D - two transitions with the same pre and post and same inhibitor arcs
        // This does not alter the trace.
        bool continueReductions = false;
        _tflags.resize(_builder->_transitions.size(), 0);
        std::fill(_tflags.begin(), _tflags.end(), 0);
        bool has_empty_trans = false;
        for(size_t t = 0; t < _builder->_transitions.size(); ++t)
        {
            auto& trans = _builder->_transitions[t];
            if(!trans._skip && trans._pre.size() == 0 && trans._post.size() == 0)
            {
                if(has_empty_trans)
                {
                    ++_ruleD;
                    skip_transition(t);
                }
                has_empty_trans = true;
            }

        }
        for(auto& op : _builder->_places)
        for(size_t outer = 0; outer < op._consumers.size(); ++outer)
        {
            auto touter = op._consumers[outer];
            if(has_timed_out()) return false;
            if(_tflags[touter] != 0) continue;
            _tflags[touter] = 1;
            Transition& tout = get_transition(touter);
            if (tout._skip) continue;

            // D2. No inhibitors
            if (tout._inhib) continue;

            for(size_t inner = outer + 1; inner < op._consumers.size(); ++inner) {
                auto tinner = op._consumers[inner];
                Transition& tin = get_transition(tinner);
                if (tin._skip || tout._skip) continue;

                // D2. No inhibitors
                if (tin._inhib) continue;

                for (size_t swp = 0; swp < 2; ++swp) {
                    if(has_timed_out()) return false;

                    if (tin._skip || tout._skip) break;

                    uint t1 = touter;
                    uint t2 = tinner;
                    if (swp == 1) std::swap(t1, t2);

                    // D1. not same transition
                    assert(t1 != t2);

                    Transition& trans1 = get_transition(t1);
                    Transition& trans2 = get_transition(t2);

                    // From D3, and D4 we have that pre and post-sets are the same
                    if (trans1._post.size() != trans2._post.size()) break;
                    if (trans1._pre.size() != trans2._pre.size()) break;

                    int ok = 0;
                    uint mult = std::numeric_limits<uint>::max();
                    // D4. postsets must match
                    for (int i = trans1._post.size() - 1; i >= 0; --i) {
                        Arc& arc = trans1._post[i];
                        Arc& arc2 = trans2._post[i];
                        if (arc2._place != arc._place) {
                            ok = 2;
                            break;
                        }

                        if (mult == std::numeric_limits<uint>::max()) {
                            if (arc2._weight < arc._weight || (arc2._weight % arc._weight) != 0) {
                                ok = 1;
                                break;
                            } else {
                                mult = arc2._weight / arc._weight;
                            }
                        } else if (arc2._weight != arc._weight * mult) {
                            ok = 2;
                            break;
                        }
                    }

                    if (ok == 2) break;
                    else if (ok == 1) continue;

                    // D3. Presets must match
                    for (int i = trans1._pre.size() - 1; i >= 0; --i) {
                        Arc& arc = trans1._pre[i];
                        Arc& arc2 = trans2._pre[i];
                        if (arc2._place != arc._place) {
                            ok = 2;
                            break;
                        }

                        if (mult == std::numeric_limits<uint>::max()) {
                            if (arc2._weight < arc._weight || (arc2._weight % arc._weight) != 0) {
                                ok = 1;
                                break;
                            } else {
                                mult = arc2._weight / arc._weight;
                            }
                        } else if (arc2._weight != arc._weight * mult) {
                            ok = 2;
                            break;
                        }
                    }

                    if (ok == 2) break;
                    else if (ok == 1) continue;

                    // UD1. Remove transition t2
                    continueReductions = true;
                    _ruleD++;
                    skip_transition(t2);
                    _tflags[touter] = 0;
                    break; // break the swap loop
                }
            }
        } // end of main for loop for rule D
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::rule_e(uint32_t* placeInQuery) {
        bool continueReductions = false;
        const size_t number_of_places = _builder->number_of_places();
        for(uint32_t p = 0; p < number_of_places; ++p)
        {
            if(has_timed_out()) return false;
            Place& place = _builder->_places[p];
            if(place._skip) continue;
            if(place._inhib) continue;
            if(place._producers.size() > place._consumers.size()) continue;

            std::set<uint32_t> notenabled;
            bool ok = true;
            for(uint cons : place._consumers)
            {
                Transition& t = get_transition(cons);
                auto in = get_in_arc(p, t);
                if(in->_weight <= _builder->_initial_marking[p])
                {
                    auto out = get_out_arc(t, p);
                    if(out == t._post.end() || out->_place != p || out->_weight >= in->_weight)
                    {
                        ok = false;
                        break;
                    }
                }
                else
                {
                    notenabled.insert(cons);
                }
            }

            if(!ok || notenabled.size() == 0) continue;

            for(uint prod : place._producers)
            {
                if(notenabled.count(prod) == 0)
                {
                    ok = false;
                    break;
                }
                // check that producing arcs originate from transition also
                // consuming. If so, we know it will never fire.
                Transition& t = get_transition(prod);
                ArcIter it = get_in_arc(p, t);
                if(it == t._pre.end())
                {
                    ok = false;
                    break;
                }
            }

            if(!ok) continue;

            _ruleE++;
            continueReductions = true;

            if(placeInQuery[p] == 0)
                _builder->_initial_marking[p] = 0;

            bool skipplace = (notenabled.size() == place._consumers.size()) && (placeInQuery[p] == 0);
            for(uint cons : notenabled)
                skip_transition(cons);

            if(skipplace)
                skip_place(p);

        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::rule_i(uint32_t* placeInQuery, bool remove_loops, bool remove_consumers) {
        bool reduced = false;
        if(remove_loops)
        {

            auto result = relevant(placeInQuery, remove_consumers);
            if (!result) {
                return false;
            }
            auto[tseen, pseen] = result.value();

            reduced |= remove_irrelevant(placeInQuery, tseen, pseen);

            if(reduced)
                ++_ruleI;
        }
        else
        {
            const size_t number_of_places = _builder->number_of_places();
            for(uint32_t p = 0; p < number_of_places; ++p)
            {
                if(has_timed_out()) return false;
                Place& place = _builder->_places[p];
                if(place._skip) continue;
                if(place._inhib) continue;
                if(placeInQuery[p] > 0) continue;
                if(place._consumers.size() > 0) continue;

                ++_ruleI;
                reduced = true;

                std::vector<uint32_t> torem;
                if(remove_consumers)
                {
                    for(auto& t : place._producers)
                    {
                        auto& trans = _builder->_transitions[t];
                        if(trans._post.size() != 1) // place will be removed later
                            continue;
                        bool ok = true;
                        for(auto& a : trans._pre)
                        {
                            if(placeInQuery[a._place] > 0)
                            {
                                ok = false;
                            }
                        }
                        if(ok) torem.push_back(t);
                    }
                }
                skip_place(p);
                for(auto t : torem)
                    skip_transition(t);
                assert(consistent());
            }
        }

        return reduced;
    }

    bool Reducer::rule_f(uint32_t* placeInQuery) {
        bool continueReductions = false;
        const size_t number_of_places = _builder->number_of_places();
        for(uint32_t p = 0; p < number_of_places; ++p)
        {
            if(has_timed_out()) return false;
            Place& place = _builder->_places[p];
            if(place._skip) continue;
            if(place._inhib) continue;
            if(place._producers.size() < place._consumers.size()) continue;
            if(placeInQuery[p] != 0) continue;

            bool ok = true;
            for(uint cons : place._consumers)
            {
                Transition& t = get_transition(cons);
                auto w = get_in_arc(p, t)->_weight;
                if(w > _builder->_initial_marking[p])
                {
                    ok = false;
                    break;
                }
                else
                {
                    auto it = get_out_arc(t, p);
                    if(it == t._post.end() ||
                       it->_place != p     ||
                       it->_weight < w)
                    {
                        ok = false;
                        break;
                    }
                }
            }

            if(!ok) continue;

            ++_ruleF;

            if((number_of_places - _removedPlaces) > 1)
            {
                if(_reconstruct_trace)
                {
                    for(auto t : place._consumers)
                    {
                        std::string tname = get_transition_name(t);
                        const ArcIter arc = get_in_arc(p, get_transition(t));
                        _extraconsume[tname].emplace_back(get_place_name(p), arc->_weight);
                    }
                }
                skip_place(p);
                continueReductions = true;
            }

        }
        assert(consistent());
        return continueReductions;
    }


    bool Reducer::rule_g(uint32_t* placeInQuery, bool remove_loops, bool remove_consumers) {
        if(!remove_loops) return false;
        bool continueReductions = false;
        for(uint32_t t = 0; t < _builder->number_of_transitions(); ++t)
        {
            if(has_timed_out()) return false;
            Transition& trans = _builder->_transitions[t];
            if(trans._skip) continue;
            if(trans._inhib) continue;
            if(trans._pre.size() < trans._post.size()) continue;
            if(!remove_loops && trans._pre.size() == 0) continue;

            auto postit = trans._post.begin();
            auto preit = trans._pre.begin();

            bool ok = true;
            while(true)
            {
                if(preit == trans._pre.end() && postit == trans._post.end())
                    break;
                if(preit == trans._pre.end())
                {
                    ok = false;
                    break;
                }
                if(preit->_inhib || _builder->_places[preit->_place]._inhib)
                {
                    ok = false;
                    break;
                }
                if(postit != trans._post.end() && preit->_place == postit->_place)
                {
                    if(!remove_consumers && preit->_weight != postit->_weight)
                    {
                        ok = false;
                        break;
                    }
                    if((placeInQuery[preit->_place] > 0 && preit->_weight != postit->_weight) ||
                       (placeInQuery[preit->_place] == 0 && preit->_weight < postit->_weight))
                    {
                        ok = false;
                        break;
                    }
                    ++preit;
                    ++postit;
                }
                else if(postit == trans._post.end() || preit->_place < postit->_place)
                {
                    if(placeInQuery[preit->_place] > 0 || !remove_consumers)
                    {
                        ok = false;
                        break;
                    }
                    ++preit;
                }
                else
                {
                    // could not match a post with a pre
                    ok = false;
                    break;
                }
            }
            if(ok)
            {
                for(preit = trans._pre.begin();preit != trans._pre.end(); ++preit)
                {
                    if(preit->_inhib || _builder->_places[preit->_place]._inhib)
                    {
                        ok = false;
                        break;
                    }
                }
            }

            if(!ok) continue;
            ++_ruleG;
            skip_transition(t);
        }
        assert(consistent());
        return continueReductions;
    }

    bool Reducer::rule_h(uint32_t* placeInQuery)
    {
        if(_reconstruct_trace)
            return false; // we don't know where in the loop the tokens are needed
        auto transok = [this](uint32_t t) -> uint32_t {
            auto& trans = _builder->_transitions[t];
            if(_tflags[t] != 0)
                return _tflags[t];
            _tflags[t] = 1;
            if(trans._inhib ||
               trans._pre.size() != 1 ||
               trans._post.size() != 1)
            {
                return 2;
            }

            auto p1 = trans._pre[0]._place;
            auto p2 = trans._post[0]._place;

            // we actually do not need weights to be 1 here.
            // there is a special case when the places are always "inputting"
            // and "outputting" with a GCD that is equal to the weight of the
            // specific transition.
            // Ie, the place always have a number of tokens (disregarding
            // initial tokens) that is dividable with the transition weight

            if(trans._pre[0]._weight != 1 ||
               trans._post[0]._weight != 1 ||
               p1 == p2 ||
               _builder->_places[p1]._inhib ||
               _builder->_places[p2]._inhib)
            {
                return 2;
            }
            return 1;
        };

        auto removeLoop = [this,placeInQuery](std::vector<uint32_t>& loop) -> bool {
            size_t i = 0;
            for(; i < loop.size(); ++i)
                if(loop[i] == loop.back())
                    break;

            assert(_tflags[loop.back()]== 1);
            if(i == loop.size() - 1)
                return false;

            auto p1 = _builder->_transitions[loop[i]]._pre[0]._place;
            bool removed = false;

            for(size_t j = i + 1; j < loop.size() - 1; ++j)
            {
                if(has_timed_out())
                    return removed;
                auto p2 = _builder->_transitions[loop[j]]._pre[0]._place;
                if(placeInQuery[p2] > 0 || placeInQuery[p1] > 0)
                {
                    p1 = p2;
                    continue;
                }
                if(p1 == p2)
                {
                    continue;
                }
                removed = true;
                ++_ruleH;
                skip_transition(loop[j-1]);
                auto& place1 = _builder->_places[p1];
                auto& place2 = _builder->_places[p2];

                {

                    for(auto p2it : place2._consumers)
                    {
                        auto& t = _builder->_transitions[p2it];
                        auto arc = get_in_arc(p2, t);
                        assert(arc != t._pre.end());
                        assert(arc->_place == p2);
                        auto a = *arc;
                        a._place = p1;
                        auto dest = std::lower_bound(t._pre.begin(), t._pre.end(), a);
                        if(dest == t._pre.end() || dest->_place != p1)
                        {
                            t._pre.insert(dest, a);
                            auto lb = std::lower_bound(place1._consumers.begin(), place1._consumers.end(), p2it);
                            place1._consumers.insert(lb, p2it);
                        }
                        else
                        {
                            dest->_weight += a._weight;
                        }
                        consistent();
                    }
                }

                {
                    auto p2it = place2._producers.begin();

                    for(;p2it != place2._producers.end(); ++p2it)
                    {
                        auto& t = _builder->_transitions[*p2it];
                        Arc a = *get_out_arc(t, p2);
                        a._place = p1;
                        auto dest = std::lower_bound(t._post.begin(), t._post.end(), a);
                        if(dest == t._post.end() || dest->_place != p1)
                        {
                            t._post.insert(dest, a);
                            auto lb = std::lower_bound(place1._producers.begin(), place1._producers.end(), *p2it);
                            place1._producers.insert(lb, *p2it);
                        }
                        else
                        {
                            dest->_weight += a._weight;
                        }
                        consistent();
                    }
                }
                _builder->_initial_marking[p1] += _builder->_initial_marking[p2];
                skip_place(p2);
                assert(placeInQuery[p2] == 0);
            }
            return removed;
        };

        bool continueReductions = false;
        for(uint32_t t = 0; t < _builder->number_of_transitions(); ++t)
        {
            if(has_timed_out())
                return continueReductions;
            _tflags.resize(_builder->_transitions.size(), 0);
            std::fill(_tflags.begin(), _tflags.end(), 0);
            std::vector<uint32_t> stack;
            {
                if(_tflags[t] != 0) continue;
                auto& trans = _builder->_transitions[t];
                if(trans._skip) continue;
                _tflags[t] = transok(t);
                if(_tflags[t] != 1) continue;
                stack.push_back(t);
            }
            bool outer = true;
            while(stack.size() > 0 && outer)
            {
                if(has_timed_out())
                    return continueReductions;
                auto it = stack.back();
                auto post = _builder->_transitions[it]._post[0]._place;
                bool found = false;
                for(auto& nt : _builder->_places[post]._consumers)
                {
                    if(has_timed_out())
                        return continueReductions;
                    auto& nexttrans = _builder->_transitions[nt];
                    if(nt == it || nexttrans._skip)
                        continue; // handled elsewhere
                    if(_tflags[nt] == 1 && stack.size() > 1)
                    {
                        stack.push_back(nt);
                        bool found = removeLoop(stack);
                        continueReductions |= found;

                        if(found)
                        {
                            outer = false;
                            break;
                        }
                        else
                        {
                            stack.pop_back();
                        }
                    }
                    else if(_tflags[nt] == 0)
                    {
                        _tflags[nt] = transok(nt);
                        if(_tflags[nt] == 2)
                        {
                            continue;
                        }
                        else
                        {
                            assert(_tflags[nt] == 1);
                            stack.push_back(nt);
                            found = true;
                            break;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
                if(!found && outer)
                {
                    _tflags[it] = 2;
                    stack.pop_back();
                }
            }
        }
        return continueReductions;
    }

    bool Reducer::rule_j(uint32_t* placeInQuery)
    {
        bool continueReductions = false;
        for(uint32_t t = 0; t < _builder->number_of_transitions(); ++t)
        {
            if(has_timed_out())
                return continueReductions;

            if(_builder->_transitions[t]._skip ||
               _builder->_transitions[t]._inhib ||
               _builder->_transitions[t]._pre.size() != 1)
                continue;
            auto p = _builder->_transitions[t]._pre[0]._place;
            if(placeInQuery[p] > 0)
            {
                continue; // can be relaxed
            }
            if(_builder->_initial_marking[p] > 0)
            {
                continue; // can be relaxed
            }
            const Place& place = _builder->_places[p];
            if(place._skip) continue;
            if(place._inhib) continue;
            if(place._consumers.size() < 1) continue;
            if(place._producers.size() < 1) continue;

            // check that prod and cons are not overlapping
            const auto presize = place._producers.size(); // can be relaxed >= 2
            const auto postsize = place._consumers.size(); // can be relaxed >= 2
            bool ok = true;
            for(size_t i = 0; i < postsize; ++i)
            {   // this can be done smarter than a quadratic loop!
                for(size_t j = 0; j < presize; ++j)
                {
                    ok &= place._consumers[i] != place._producers[j];
                }
            }
            if(!ok) continue;
            // check that post of consumer is not messing with query or inhib
            // if either all pre or all post are query-free, we are ok.
            bool inquery = false;
            for(auto t : place._consumers)
            {
                Transition& trans = _builder->_transitions[t];
                if(trans._pre.size() == 1) // can be relaxed
                {
                    // check that weights match
                    // can be relaxed
                    ok &= trans._pre[0]._weight == 1;
                    ok &= !trans._pre[0]._inhib;
                }
                else
                {
                    ok = false;
                    break;
                }
                for(auto& pp : trans._post)
                {
                    ok &= !_builder->_places[pp._place]._inhib;
                    inquery |= placeInQuery[pp._place] > 0;
                    ok &= pp._weight == 1; // can be relaxed
                }
                if(!ok)
                    break;
            }
            if(!ok) continue;
            // check that pre of producing do not mess with query or inhib
            for(auto& t : place._producers)
            {
                Transition& trans = _builder->_transitions[t];
                for(const auto& arc : trans._post)
                {
                    ok &= !inquery || placeInQuery[arc._place] == 0;
                    ok &= !_builder->_places[arc._place]._inhib;
                }
            }
            if(!ok) continue;
            ++_ruleJ;
            continueReductions = true;
            // otherwise we can skip the place by merging up the two transitions
            // constructing 4 new transitions, one for each combination.
            // In the binary case, we want to achieve the following four transitions
            // post[n] = pre[n] + post[n]
            // pre[0] = pre[0] + post[1]
            // pre[1] = pre[1] + post[0]

            // start by copying out the post of each of the posts
            Place pp = place;
            skip_place(p);
            std::vector<std::vector<Arc>> posts;
            std::vector<Transition> pres;

            for(auto t : pp._consumers)
                posts.push_back(_builder->_transitions[t]._post);

            for(auto t : pp._producers)
                pres.push_back(_builder->_transitions[t]);

            // remove old transitions, we will create new ones
            for(auto t : pp._consumers)
                skip_transition(t);

            for(auto t : pp._producers)
                skip_transition(t);

            // compute all permutations
            for(auto& trans : pres)
            {
                for(auto& postset : posts)
                {
                    auto id = _builder->_transitions.size();
                    if(!_skipped_trans.empty())
                        id = _skipped_trans.back();
                    else
                    {
                        continue;
                        _builder->_transitions.emplace_back();
                    }
                    _builder->_transitions[id] = trans;
                    auto& target = _builder->_transitions[id];
                    for(auto& arc : postset)
                        target.add_post_arc(arc);

                    // add to places
                    if(_skipped_trans.empty())
                        _builder->_transitionnames[new_trans_name()] = id;

                    for(auto& arc : target._pre)
                        _builder->_places[arc._place].add_consumer(id);
                    for(auto& arc : target._post)
                        _builder->_places[arc._place].add_producer(id);
                    if(!_skipped_trans.empty())
                    {
                        --_removedTransitions; // recycling
                        _skipped_trans.pop_back();
                    }
                    _builder->_transitions[id]._skip = false;
                    _builder->_transitions[id]._inhib = false;
                    consistent();
                }
            }
            consistent();
        }
        return continueReductions;
    }

    bool Reducer::rule_k(uint32_t *placeInQuery, bool remove_consumers) {
        bool reduced = false;
        auto opt = relevant(placeInQuery, remove_consumers);
        if (!opt)
            return false;
        auto[tseen, pseen] = opt.value();
        for (std::size_t p = 0; p < _builder->number_of_places(); ++p) {
            if (placeInQuery[p] != 0)
                pseen[p] = true;
        }

        for (std::size_t t = 0; t < _builder->number_of_transitions(); ++t) {
            auto transition = _builder->_transitions[t];
            if (!tseen[t] && !transition._skip && !transition._inhib && transition._pre.size() == 1 &&
                transition._post.size() == 1
                && transition._pre[0]._place == transition._post[0]._place) {
                auto p = transition._pre[0]._place;
                if (!pseen[p] && !_builder->_places[p]._inhib) {
                    if (_builder->_initial_marking[p] >= transition._pre[0]._weight){
                        //Mark the initially marked self loop as relevant.
                        tseen[t] = true;
                        pseen[p] = true;
                        reduced |= remove_irrelevant(placeInQuery, tseen, pseen);
                        _ruleK++;
                        return reduced;
                    }
                    if (transition._pre[0]._weight == 1){
                        for (auto t2 : _builder->_places[p]._consumers) {
                            auto transition2 = _builder->_transitions[t2];
                            if (t != t2 && !tseen[t2] && !transition2._skip) {
                                skip_transition(t2);
                                reduced = true;
                                _ruleK++;
                            }
                        }
                    }
                }
            }
        }
        return reduced;
    }

    std::optional<std::pair<std::vector<bool>, std::vector<bool>>>
    Reducer::relevant(const uint32_t *placeInQuery, bool remove_consumers) {
        std::vector<uint32_t> wtrans;
        std::vector<bool> tseen(_builder->number_of_transitions(), false);
        for (uint32_t p = 0; p < _builder->number_of_places(); ++p) {
            if (has_timed_out()) return std::nullopt;
            if (placeInQuery[p] > 0) {
                const Place &place = _builder->_places[p];
                for (auto t : place._consumers) {
                    if (!tseen[t]) {
                        wtrans.push_back(t);
                        tseen[t] = true;
                    }
                }
                for (auto t : place._producers) {
                    if (!tseen[t]) {
                        wtrans.push_back(t);
                        tseen[t] = true;
                    }
                }
            }
        }
        std::vector<bool> pseen(_builder->number_of_places(), false);

        while (!wtrans.empty()) {
            if (has_timed_out()) return std::nullopt;
            auto t = wtrans.back();
            wtrans.pop_back();
            const Transition &trans = _builder->_transitions[t];
            for (const Arc &arc : trans._pre) {
                const Place &place = _builder->_places[arc._place];
                if (arc._inhib) {
                    for (auto pt : place._consumers) {
                        if (!tseen[pt]) {
                            Transition &trans = _builder->_transitions[pt];
                            auto it = trans._post.begin();
                            for (; it != trans._post.end(); ++it)
                                if (it->_place >= arc._place) break;
                            if (it != trans._post.end() && it->_place == arc._place) {
                                auto it2 = trans._pre.begin();
                                for (; it2 != trans._pre.end(); ++it2)
                                    if (it2->_place >= arc._place || it2->_inhib) break;
                                if (it->_weight <= it2->_weight) continue;
                            }
                            tseen[pt] = true;
                            wtrans.push_back(pt);
                        }
                    }
                } else {
                    for (auto pt : place._producers) {
                        if (!tseen[pt]) {
                            Transition &trans = _builder->_transitions[pt];
                            auto it = trans._pre.begin();
                            for (; it != trans._pre.end(); ++it)
                                if (it->_place >= arc._place) break;

                            if (it != trans._pre.end() && it->_place == arc._place && !it->_inhib) {
                                auto it2 = trans._post.begin();
                                for (; it2 != trans._post.end(); ++it2)
                                    if (it2->_place >= arc._place) break;
                                if (it->_weight >= it2->_weight) continue;
                            }

                            tseen[pt] = true;
                            wtrans.push_back(pt);
                        }
                    }

                    for (auto pt : place._consumers) {
                        if (!tseen[pt] && (!remove_consumers || placeInQuery[arc._place] > 0)) {
                            tseen[pt] = true;
                            wtrans.push_back(pt);
                        }
                    }
                }
                pseen[arc._place] = true;
            }
        }
        return std::make_optional(std::pair(tseen, pseen));
    }

    bool Reducer::remove_irrelevant(const uint32_t* placeInQuery, const std::vector<bool> &tseen, const std::vector<bool> &pseen) {
        bool reduced = false;
        for(size_t t = 0; t < _builder->number_of_transitions(); ++t)
        {
            if(!tseen[t] && !_builder->_transitions[t]._skip)
            {
                skip_transition(t);
                reduced = true;
            }
        }

        for(size_t p = 0; p < _builder->number_of_places(); ++p)
        {
            if(!pseen[p] && !_builder->_places[p]._skip && placeInQuery[p] == 0)
            {
                assert(placeInQuery[p] == 0);
                skip_place(p);
                reduced = true;
            }
        }
        return reduced;
    }

    std::array tnames {
            "T-lb_balancing_receive_notification_10",
            "T-lb_balancing_receive_notification_2",
            "T-lb_balancing_receive_notification_3",
            "T-lb_balancing_receive_notification_8",
            "T-lb_balancing_receive_notification_9",
            "T-lb_idle_receive_notification_4",
            "T-lb_no_balance_1",
            "T-lb_receive_client_1",
            "T-lb_receive_client_2",
            "T-lb_receive_client_3",
            "T-lb_receive_client_5",
            "T-lb_route_to_1_1",
            "T-lb_route_to_1_8",
            "T-lb_route_to_1_87",
            "T-lb_route_to_2_165",
            "T-lb_route_to_2_43",
            "T-lb_route_to_2_50",
            "T-server_endloop_1",
            "T-server_endloop_2",
            "T-server_process_1",
            "T-server_process_10",
            "T-server_process_3",
            "T-server_process_7"
    };

    void Reducer::reduce(QueryPlaceAnalysisContext& context, int enablereduction, bool reconstructTrace, int timeout, bool remove_loops, bool remove_consumers, bool next_safe, std::vector<uint32_t>& reduction) {

        this->_timeout = timeout;
        _timer = std::chrono::high_resolution_clock::now();
        assert(consistent());
        this->_reconstruct_trace = reconstructTrace;
        if(reconstructTrace && enablereduction >= 1 && enablereduction <= 2)
            std::cout << "Rule H disabled when a trace is requested." << std::endl;
        if (enablereduction == 2) { // for k-boundedness checking only rules A, D and H are applicable
            bool changed = true;
            while (changed && !has_timed_out()) {
                changed = false;
                if(!next_safe)
                {
                    while(rule_a(context.get_query_placeCount())) changed = true;
                    while(rule_d(context.get_query_placeCount())) changed = true;
                    while(rule_h(context.get_query_placeCount())) changed = true;
                }
            }
        }
        else if (enablereduction == 1) { // in the aggressive reduction all four rules are used as long as they remove something
            bool changed = false;
            do
            {
                if(remove_loops && !next_safe)
                    while(rule_i(context.get_query_placeCount(), remove_loops, remove_consumers)) changed = true;
                do{
                    do { // start by rules that do not move tokens
                        changed = false;
                        while(rule_e(context.get_query_placeCount())) changed = true;
                        while(rule_c(context.get_query_placeCount())) changed = true;
                        while(rule_f(context.get_query_placeCount())) changed = true;
                        if(!next_safe)
                        {
                            while(rule_g(context.get_query_placeCount(), remove_loops, remove_consumers)) changed = true;
                            if(!remove_loops)
                                while(rule_i(context.get_query_placeCount(), remove_loops, remove_consumers)) changed = true;
                            while(rule_d(context.get_query_placeCount())) changed = true;
                            //changed |= ReducebyRuleK(context.getQueryPlaceCount(), remove_consumers); //Rule disabled as correctness has not been proved. Experiments indicate that it is not correct for CTL.
                        }
                    } while(changed && !has_timed_out());
                    if(!next_safe)
                    { // then apply tokens moving rules
                        //while(ReducebyRuleJ(context.getQueryPlaceCount())) changed = true;
                        while(rule_b(context.get_query_placeCount(), remove_loops, remove_consumers)) changed = true;
                        while(rule_a(context.get_query_placeCount())) changed = true;
                    }
                } while(changed && !has_timed_out());
                if(!next_safe && !changed)
                {
                    // Only try RuleH last. It can reduce applicability of other rules.
                    while(rule_h(context.get_query_placeCount())) changed = true;
                }
            } while(!has_timed_out() && changed);

        }
        else
        {
            const char* rnames = "ABCDEFGHIJK";
            for(int i = reduction.size() - 1; i >= 0; --i)
            {
                if(next_safe)
                {
                    if(reduction[i] != 2 && reduction[i] != 4 && reduction[i] != 5)
                    {
                        std::cerr << "Skipping Rule" << rnames[reduction[i]] << " due to NEXT operator in proposition" << std::endl;
                        reduction.erase(reduction.begin() + i);
			            continue;
                    }
                }
                if(!remove_loops && reduction[i] == 5)
                {
                    std::cerr << "Skipping Rule" << rnames[reduction[i]] << " as proposition is loop sensitive" << std::endl;
                    reduction.erase(reduction.begin() + i);
                }
            }
            bool changed = true;
            while(changed && !has_timed_out())
            {
                changed = false;
                for(auto r : reduction)
                {
#ifndef NDEBUG
                    auto c = std::chrono::high_resolution_clock::now();
                    auto op = _removedPlaces;
                    auto ot = _removedTransitions;
#endif
                    switch(r)
                    {
                        case 0:
                            while(rule_a(context.get_query_placeCount())) changed = true;
                            break;
                        case 1:
                            while(rule_b(context.get_query_placeCount(), remove_loops, remove_consumers)) changed = true;
                            break;
                        case 2:
                            while(rule_c(context.get_query_placeCount())) changed = true;
                            break;
                        case 3:
                            while(rule_d(context.get_query_placeCount())) changed = true;
                            break;
                        case 4:
                            while(rule_e(context.get_query_placeCount())) changed = true;
                            break;
                        case 5:
                            while(rule_f(context.get_query_placeCount())) changed = true;
                            break;
                        case 6:
                            while(rule_g(context.get_query_placeCount(), remove_loops, remove_consumers)) changed = true;
                            break;
                        case 7:
                            while(rule_h(context.get_query_placeCount())) changed = true;
                            break;
                        case 8:
                            while(rule_i(context.get_query_placeCount(), remove_loops, remove_consumers)) changed = true;
                            break;
                        case 9:
                            while(rule_j(context.get_query_placeCount())) changed = true;
                            break;
                        case 10:
                            if (rule_k(context.get_query_placeCount(), remove_consumers)) changed = true;
                            break;
                    }
#ifndef NDEBUG
                    auto end = std::chrono::high_resolution_clock::now();
                    auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - c);
                    std::cout << "SPEND " << diff.count()  << " ON " << rnames[r] << std::endl;
                    std::cout << "REM " << ((int)_removedPlaces-(int)op) << " " << ((int)_removedTransitions-(int)ot) << std::endl;
#endif
                    if(has_timed_out())
                        break;
                }
            }
        }

        return;
        std::vector<std::string> names(_builder->number_of_transitions());
        for (auto &entry : _builder->_transitionnames) {
            names[entry.second] = entry.first;
        }
        for (size_t i = 0; i < _builder->number_of_transitions(); i++) {
            auto tName = names[i];
            if (std::find(std::begin(tnames), std::end(tnames), tName) == std::end(tnames)) {
                if (!get_transition(i)._skip)
                    skip_transition(i);
            }
            else {
                std::cerr << "Including " << tName << std::endl;
            }
        }

    }

    void Reducer::post_fire(std::ostream& out, const std::string& transition) const
    {
        if(_postfire.count(transition) > 0)
        {
            std::queue<std::string> tofire;

            for(auto& el : _postfire.at(transition)) tofire.push(el);

            for(auto& el : _postfire.at(transition))
            {
                tofire.pop();
                out << "\t<transition id=\"" << el << "\">\n";
                extra_consume(out, el);
                out << "\t</transition>\n";
                post_fire(out, el);
            }
        }
    }

    void Reducer::init_fire(std::ostream& out) const
    {
        for(auto& init : _initfire)
        {
            out << "\t<transition id=\"" << init << "\">\n";
            extra_consume(out, init);
            out << "\t</transition>\n";
            post_fire(out, init);
        }
    }

    void Reducer::extra_consume(std::ostream& out, const std::string& transition) const
    {
        if(_extraconsume.count(transition) > 0)
        {
            for(auto& ec : _extraconsume.at(transition))
            {
                out << ec;
            }
        }
    }

} //PetriNet namespace
