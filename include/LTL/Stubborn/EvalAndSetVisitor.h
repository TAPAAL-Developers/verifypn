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

#ifndef VERIFYPN_EVALANDSETVISITOR_H
#define VERIFYPN_EVALANDSETVISITOR_H

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/MutatingVisitor.h"

namespace LTL {

class EvalAndSetVisitor : public PetriEngine::PQL::MutatingVisitor {

  public:
    EvalAndSetVisitor(const PetriEngine::PQL::EvaluationContext &context) : _context(context) {}

  private:
    const PetriEngine::PQL::EvaluationContext &_context;

  protected:
  protected:
    void accept(PetriEngine::PQL::ACondition *condition) override;

    void accept(PetriEngine::PQL::ECondition *condition) override;

    void accept(PetriEngine::PQL::GCondition *condition) override;

    void accept(PetriEngine::PQL::FCondition *condition) override;

    void accept(PetriEngine::PQL::XCondition *condition) override;

    void accept(PetriEngine::PQL::UntilCondition *condition) override;

    void accept(PetriEngine::PQL::NotCondition *element) override;

    void accept(PetriEngine::PQL::AndCondition *element) override;

    void accept(PetriEngine::PQL::OrCondition *element) override;

    void accept(PetriEngine::PQL::LessThanCondition *element) override;

    void accept(PetriEngine::PQL::LessThanOrEqualCondition *element) override;

    void accept(PetriEngine::PQL::EqualCondition *element) override;

    void accept(PetriEngine::PQL::NotEqualCondition *element) override;

    void accept(PetriEngine::PQL::DeadlockCondition *element) override;

    void accept(PetriEngine::PQL::CompareConjunction *element) override;

    void accept(PetriEngine::PQL::UnfoldedUpperBoundsCondition *element) override;

    void accept(PetriEngine::PQL::BooleanCondition *element) override;
};

} // namespace LTL

#endif // VERIFYPN_EVALANDSETVISITOR_H
