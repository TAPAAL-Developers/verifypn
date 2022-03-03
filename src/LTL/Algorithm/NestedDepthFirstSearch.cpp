/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LTL/Algorithm/NestedDepthFirstSearch.h"

namespace LTL {
    template<typename S>
    bool NestedDepthFirstSearch<S>::isSatisfied()
    {
        this->is_weak = this->successorGenerator->is_weak() && this->shortcircuitweak;
        dfs();
        return !_violation;
    }

    template<typename S>
    std::pair<bool,size_t> NestedDepthFirstSearch<S>::mark(State& state, const uint8_t MARKER)
    {
        auto[_, stateid] = _states.add(state);
        if (stateid == std::numeric_limits<size_t>::max()) {
            return std::make_pair(false, stateid);
        }

        auto r = _markers[stateid];
        _markers[stateid] = (MARKER | r);
        const bool is_new = (r & MARKER) == 0;
        if(is_new)
        {
            ++_mark_count[MARKER];
            ++this->_discovered;
        }
        return std::make_pair(is_new, stateid);
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::dfs()
    {

        light_deque<StackEntry> todo;
        light_deque<StackEntry> nested_todo;

        State working = this->_factory.newState();
        State curState = this->_factory.newState();

        {
            std::vector<State> initial_states = this->successorGenerator->makeInitialState();
            for (auto &state : initial_states) {
                auto res = _states.add(state);
                if (res.first) {
                    todo.push_back(StackEntry{res.second, S::initial_suc_info()});
                    ++this->_discovered;
                }
            }
        }

        while (!todo.empty()) {
            auto &top = todo.back();
            _states.decode(curState, top._id);
            this->successorGenerator->prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo.last_state);
            }
            if (!this->successorGenerator->next(working, top._sucinfo)) {
                // no successor
                if (curState.is_accepting()) {
                    if(this->successorGenerator->has_invariant_self_loop(curState))
                        _violation = true;
                    else
                        ndfs(curState, nested_todo);
                    if (_violation) {
                        if (_print_trace) {
                            print_trace(todo, nested_todo);
                        }
                        return;
                    }
                }
                todo.pop_back();
            } else {
                auto [is_new, stateid] = mark(working, MARKER1);
                if (stateid == std::numeric_limits<size_t>::max()) {
                    continue;
                }
                top._sucinfo.last_state = stateid;
                if (is_new) {
                    ++this->_discovered;
                    if(this->successorGenerator->isAccepting(curState) &&
                       this->successorGenerator->has_invariant_self_loop(curState))
                    {
                        _violation = true;
                        if(_print_trace)
                            print_trace(todo, nested_todo);
                        return;
                    }
                    todo.push_back(StackEntry{stateid, S::initial_suc_info()});
                }
            }
        }
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::ndfs(const State &state, light_deque<StackEntry>& nested_todo)
    {

        State working = this->_factory.newState();
        State curState = this->_factory.newState();

        nested_todo.push_back(StackEntry{_states.add(state).second, S::initial_suc_info()});

        while (!nested_todo.empty()) {
            auto &top = nested_todo.back();
            _states.decode(curState, top._id);
            this->successorGenerator->prepare(&curState, top._sucinfo);
            if (top._sucinfo.has_prev_state()) {
                _states.decode(working, top._sucinfo.last_state);
            }
            if (!this->successorGenerator->next(working, top._sucinfo)) {
                nested_todo.pop_back();
            } else {
                if (this->is_weak && !this->successorGenerator->isAccepting(working)) {
                    continue;
                }
                if (working == state) {
                    _violation = true;
                    return;
                }
                auto [is_new, stateid] = mark(working, MARKER2);
                if (stateid == std::numeric_limits<size_t>::max())
                    continue;
                top._sucinfo.last_state = stateid;
                if (is_new) {
                    nested_todo.push_back(StackEntry{stateid, S::initial_suc_info()});
                }
            }
        }
    }

    template<typename S>
    void NestedDepthFirstSearch<S>::printStats(std::ostream &os)
    {
        std::cout << "STATS:\n"
                  << "\tdiscovered states:          " << _states.discovered() << std::endl
                  << "\tmax tokens:                 " << _states.max_tokens() << std::endl
                  << "\texplored states:            " << _mark_count[MARKER1] << std::endl
                  << "\texplored states (nested):   " << _mark_count[MARKER2] << std::endl;
    }


    template<typename S>
    void NestedDepthFirstSearch<S>::print_trace(light_deque<StackEntry>& _todo, light_deque<StackEntry>& _nested_todo, std::ostream &os)
    {
        os << "<trace>\n";
        if(this->_reducer)
            this->_reducer->initFire(os);
        size_t loop_id = std::numeric_limits<size_t>::max();
        // last element of todo-stack always has a "garbage" transition, it is the
        // current working element OR first element of nested.

        if(!_todo.empty())
            _todo.pop_back();
        if(!_nested_todo.empty()) {
            // here the last state is significant
            // of the successor is the check that demonstrates the violation.
            loop_id = _nested_todo.back()._id;
            _nested_todo.pop_back();
        }

        for(auto* stck : {&_todo, &_nested_todo})
        {
            while(!(*stck).empty())
            {
                auto& top = (*stck).front();
                if(top._id == loop_id)
                {
                    this->printLoop(os);
                    loop_id = std::numeric_limits<size_t>::max();
                }
                this->printTransition(top._sucinfo.transition(), os) << std::endl;
                (*stck).pop_front();
            }
        }
        os << std::endl << "</trace>" << std::endl;
    }

    template
    class NestedDepthFirstSearch<LTL::ResumingSuccessorGenerator>;

    template
    class NestedDepthFirstSearch<LTL::SpoolingSuccessorGenerator>;
}
