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


#ifndef VERIFYPN_CTLHEURISTICVISITOR_H
#define VERIFYPN_CTLHEURISTICVISITOR_H

#include "PetriEngine/PQL/Visitor.h"



class CTLHeuristicVisitor : public PetriEngine::PQL::BaseVisitor {
public:
    enum BoolBehaviour {
        PeterVal,  // and -> min, or -> max
        JiriVal,   // and -> max, or -> min
        NikolajVal // and -> sum, or -> sum
    };

    explicit CTLHeuristicVisitor(BoolBehaviour boolBehaviour = BoolBehaviour::JiriVal)
            : _bool_behaviour(boolBehaviour) {}
    [[nodiscard]] int value() { return _cur_val; }

    [[nodiscard]] int eval(PetriEngine::PQL::Condition* cond) {
        cond->visit(*this);
        return _cur_val;
    }
protected:
    void _accept(const PetriEngine::PQL::NotCondition *element) override;

    void _accept(const PetriEngine::PQL::AndCondition *element) override;

    void _accept(const PetriEngine::PQL::OrCondition *element) override;

    void _accept(const PetriEngine::PQL::EFCondition *condition) override;

    void _accept(const PetriEngine::PQL::EGCondition *condition) override;

    void _accept(const PetriEngine::PQL::AGCondition *condition) override;

    void _accept(const PetriEngine::PQL::AFCondition *condition) override;

    void _accept(const PetriEngine::PQL::EXCondition *condition) override;

    void _accept(const PetriEngine::PQL::AXCondition *condition) override;

    void _accept(const PetriEngine::PQL::EUCondition *condition) override;

    void _accept(const PetriEngine::PQL::AUCondition *condition) override;

    void _accept(const PetriEngine::PQL::DeadlockCondition *element) override;

    void _accept(const PetriEngine::PQL::CompareConjunction *element) override;

    void _accept(const PetriEngine::PQL::BooleanCondition *element) override;

private:
    int _cur_val;


    BoolBehaviour _bool_behaviour = BoolBehaviour::JiriVal;

    void _accum_max(const PetriEngine::PQL::LogicalCondition *cond);

    void _accum_min(const PetriEngine::PQL::LogicalCondition *cond);

    void _accum_sum(const PetriEngine::PQL::LogicalCondition *cond);
};


#endif //VERIFYPN_CTLHEURISTICVISITOR_H
