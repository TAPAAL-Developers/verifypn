/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#include "LTL/Stubborn/SafeAutStubbornSet.h"
#include "LTL/Stubborn/VisibleTransitionVisitor.h"

namespace LTL {
    using namespace PetriEngine;

    bool SafeAutStubbornSet::prepare(const LTL::Structures::ProductState *state)
    {
        reset();
        _parent = state;
        memset(_places_seen.get(), 0, _net.number_of_places());

        constructEnabled();
        if (_ordering.empty()) {
            _print_debug();
            return false;
        }
        if (_ordering.size() == 1) {
            _stubborn[_ordering.front()] = true;
            _print_debug();
            return true;
        }

        InterestingLTLTransitionVisitor unsafe{*this, false};
        InterestingTransitionVisitor interesting{*this, false};

        PQL::EvaluationContext ctx((*_parent).marking(), &_net);
        _prog_cond->evalAndSet(ctx);
        _sink_cond->evalAndSet(ctx);
        _prog_cond->visit(unsafe);
        _sink_cond->visit(unsafe);

        //_ret_cond->evalAndSet(ctx);
        //(std::make_shared<PetriEngine::PQL::NotCondition>(_ret_cond))->visit(interesting);

        assert(!_bad);

        _unsafe.swap(_stubborn);
        _has_enabled_stubborn = false;
        //memset(_stubborn.get(), false, sizeof(bool) * _net.number_of_transitions());
        _unprocessed.clear();
        memset(_places_seen.get(), 0, _net.number_of_places());

        assert(_unprocessed.empty());



        // sink condition is not interesting, just unsafe.
        _prog_cond->visit(interesting);
        //_sink_cond->visit(interesting);
        closure();
        if (_bad) {
            // abort
            set_all_stubborn();
            _print_debug();
            return true;
        }
        // accepting states need key transition. add firs   t enabled by index.
        if (state->is_accepting() && !_has_enabled_stubborn) {
            //set_all_stubborn();
            //_print_debug();
            //return true;
            addToStub(_ordering.front());
            closure();
/*            for (int i = 0; i < _net.number_of_places(); ++i) {
                if (_enabled[i]) {
                    addToStub(i);
                    closure();
                    break;
                }
            }*/
            if (_bad) {
                set_all_stubborn();
            }
        }
        _print_debug();
        return true;
    }

    void SafeAutStubbornSet::_print_debug() {
#ifndef NDEBUG
        float num_stubborn = 0;
        float num_enabled = 0;
        float num_enabled_stubborn = 0;
        for (int i = 0; i < _net.number_of_transitions(); ++i) {
            if (_stubborn[i]) ++num_stubborn;
            if (_enabled[i]) ++num_enabled;
            if (_stubborn[i] && _enabled[i]) ++num_enabled_stubborn;
        }
        std::cerr << "Enabled: " << num_enabled << "/" << _net.number_of_transitions() << " (" << num_enabled/_net.number_of_transitions()*100.0 << "%),\t\t "
        << "Stubborn: " << num_stubborn << "/" << _net.number_of_transitions() << " (" << num_stubborn/_net.number_of_transitions()*100.0 << "%),\t\t "
        << "Enabled stubborn: " << num_enabled_stubborn << "/" << num_enabled << " (" << num_enabled_stubborn/num_enabled*100.0 << "%)" << std::endl;
#endif
    }
}
