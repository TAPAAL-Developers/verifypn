/* Copyright (C) 2022  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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


#include "CTL/DependencyGraph/CTLHeuristicVisitor.h"

#include "PetriEngine/PQL/PredicateCheckers.h"


void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::NotCondition *element) {
    element->getCond()->visit(*this);
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::AndCondition *element) {
    switch (type) {
        case CTLHeuristic::PeterVal:
            _accum_min(element);
            break;
        case CTLHeuristic::JiriVal:
            _accum_max(element);
            break;
        case CTLHeuristic::NikolajVal:
            _accum_sum(element);
            break;
    }
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::OrCondition *element) {
    switch (type) {
        case CTLHeuristic::PeterVal:
            _accum_max(element);
            break;
        case CTLHeuristic::JiriVal:
            _accum_min(element);
            break;
        case CTLHeuristic::NikolajVal:
            _accum_sum(element);
            break;
    }
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::EFCondition *condition) {
    if (isReachability(condition)) {
        _cur_val = 1; return;
    }
    condition->getCond()->visit(*this);
    _cur_val++;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::EGCondition *condition) {
    condition->getCond()->visit(*this);
    _cur_val++;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::AGCondition *condition) {
    if (isReachability(condition)) {
        _cur_val = 1; return;
    }
    condition->getCond()->visit(*this);
    _cur_val++;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::AFCondition *condition) {
    condition->getCond()->visit(*this);
    _cur_val++;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::EXCondition *condition) {
    condition->getCond()->visit(*this);
    _cur_val++;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::AXCondition *condition) {
    condition->getCond()->visit(*this);
    _cur_val++;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::EUCondition *condition) {
    // h(p U q) -> h(p) + h(q)
    (*condition)[0]->visit(*this);
    int v1 = _cur_val;
    (*condition)[1]->visit(*this);
    _cur_val += v1;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::AUCondition *condition) {
    // h(p U q) -> h(p) + h(q)
    (*condition)[0]->visit(*this);
    int v1 = _cur_val;
    (*condition)[1]->visit(*this);
    _cur_val += v1;
}

void CTLHeuristicVisitor::_accum_max(const PetriEngine::PQL::LogicalCondition *cond) {
    int acc = -1;
    for (auto& c : cond->getOperands()) {
        c->visit(*this);
        acc = std::max(acc, _cur_val);
    }
    _cur_val = acc;
}

void CTLHeuristicVisitor::_accum_min(const PetriEngine::PQL::LogicalCondition *cond) {
    int acc = std::numeric_limits<int>::max();
    for (auto& c : cond->getOperands()) {
        c->visit(*this);
        acc = std::min(acc, _cur_val);
    }
    _cur_val = acc;
}

void CTLHeuristicVisitor::_accum_sum(const PetriEngine::PQL::LogicalCondition *cond) {
    int acc = 0;
    for (auto& c : cond->getOperands()) {
        c->visit(*this);
        acc += _cur_val;
    }
    _cur_val = acc;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::DeadlockCondition *element) {
    _cur_val = 0;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::CompareConjunction *element) {
    _cur_val = 0;
}

void CTLHeuristicVisitor::_accept(const PetriEngine::PQL::BooleanCondition *element) {
    _cur_val = 0;
}
