/* Copyright (C) 2023  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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


#ifndef VERIFYPN_EXISTENTIALNORMALFORM_H
#define VERIFYPN_EXISTENTIALNORMALFORM_H

#include "MutatingVisitor.h"

namespace PetriEngine::PQL {

    Condition_ptr ctl_to_enf(Condition_ptr cond);

    class ExistentialNormalForm : public MutatingVisitor {
    public:
        Condition_ptr return_value;

    protected:

#ifndef NDEBUG
        bool has_returned = false;
#endif

        void _accept(NotCondition* element) override;

        void _accept(AndCondition* element) override;

        void _accept(OrCondition* element) override;

        void _accept(LessThanCondition* element) override;

        void _accept(LessThanOrEqualCondition* element) override;

        void _accept(EqualCondition* element) override;

        void _accept(NotEqualCondition* element) override;

        void _accept(DeadlockCondition* element) override;

        void _accept(CompareConjunction* element) override;

        void _accept(UnfoldedUpperBoundsCondition* element) override;

        void _accept(EFCondition* condition) override;

        void _accept(EGCondition* condition) override;

        void _accept(AGCondition* condition) override;

        void _accept(AFCondition* condition) override;

        void _accept(EXCondition* condition) override;

        void _accept(AXCondition* condition) override;

        void _accept(EUCondition* condition) override;

        void _accept(AUCondition* condition) override;

        void _accept(BooleanCondition* element) override;


        Condition_ptr subvisit(Condition_ptr cond, bool negated) { return subvisit(cond.get(), negated); }
        Condition_ptr subvisit(Condition* cond, bool negated);

        Condition_ptr push_and(const std::vector<Condition_ptr>& conds, bool negated);

        Condition_ptr push_or(const std::vector<Condition_ptr>& conds, bool negated);

        bool _negated = false;

        //        void _accept(NotCondition *element) override;
//        void _accept(AndCondition *element) override;
//        void _accept(OrCondition *element) override;
//        void _accept(LessThanCondition *element) override;
//        void _accept(LessThanOrEqualCondition *element) override;
//        void _accept(EqualCondition *element) override;
//        void _accept(NotEqualCondition *element) override;
//        void _accept(DeadlockCondition *element) override;
//        void _accept(CompareConjunction *element) override;
//        void _accept(UpperBoundsCondition *element) override;
//        void _accept(UnfoldedUpperBoundsCondition *element) override;
//        void _accept(ControlCondition *element) override;
//        void _accept(EFCondition *condition) override;
//        void _accept(EGCondition *condition) override;
//        void _accept(AGCondition *condition) override;
//        void _accept(AFCondition *condition) override;
//        void _accept(EXCondition *condition) override;
//        void _accept(AXCondition *condition) override;
//        void _accept(EUCondition *condition) override;
//        void _accept(AUCondition *condition) override;
//        void _accept(ACondition *condition) override;
//        void _accept(ECondition *condition) override;
//        void _accept(GCondition *condition) override;
//        void _accept(FCondition *condition) override;
//        void _accept(XCondition *condition) override;
//        void _accept(UntilCondition *condition) override;
//        void _accept(UnfoldedFireableCondition *element) override;
//        void _accept(BooleanCondition *element) override;
//        void _accept(FireableCondition* element) override;
//        void _accept(ShallowCondition *element);
//        void _accept(KSafeCondition* element) override;
//        void _accept(LivenessCondition* element) override;
//        void _accept(QuasiLivenessCondition* element) override;
//        void _accept(StableMarkingCondition* element) override;
//        void _accept(PathSelectCondition* element) override;
   };
}

#endif //VERIFYPN_EXISTENTIALNORMALFORM_H
