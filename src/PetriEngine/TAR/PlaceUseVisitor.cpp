/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   PlaceUseVisitor.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 23, 2020, 8:44 PM
 */

#include "PetriEngine/TAR/PlaceUseVisitor.h"

namespace PetriEngine::PQL {

PlaceUseVisitor::PlaceUseVisitor(size_t places) : _in_use(places) {}

void PlaceUseVisitor::accept(const NotCondition *element) { (*element)[0]->visit(*this); }

void PlaceUseVisitor::accept(const AndCondition *element) {
    for (auto &e : *element)
        e->visit(*this);
}

void PlaceUseVisitor::accept(const OrCondition *element) {
    for (auto &e : *element)
        e->visit(*this);
}

void PlaceUseVisitor::accept(const LessThanCondition *element) {
    for (auto i : {0, 1})
        (*element)[i]->visit(*this);
}

void PlaceUseVisitor::accept(const LessThanOrEqualCondition *element) {
    for (auto i : {0, 1})
        (*element)[i]->visit(*this);
}

void PlaceUseVisitor::accept(const EqualCondition *element) {
    for (auto i : {0, 1})
        (*element)[i]->visit(*this);
}

void PlaceUseVisitor::accept(const NotEqualCondition *element) {
    for (auto i : {0, 1})
        (*element)[i]->visit(*this);
}

void PlaceUseVisitor::accept(const CompareConjunction *element) {
    for (auto &e : *element)
        _in_use[e._place] = true;
}

void PlaceUseVisitor::visit_commutative_expr(const CommutativeExpr *element) {
    for (auto &p : element->places())
        _in_use[p.first] = true;
    for (auto &e : element->expressions())
        e->visit(*this);
}

void PlaceUseVisitor::accept(const PlusExpr *element) { visit_commutative_expr(element); }

void PlaceUseVisitor::accept(const SubtractExpr *element) {
    for (auto &e : element->expressions())
        e->visit(*this);
}

void PlaceUseVisitor::accept(const MultiplyExpr *element) { visit_commutative_expr(element); }

void PlaceUseVisitor::accept(const MinusExpr *element) { (*element)[0]->visit(*this); }

void PlaceUseVisitor::accept(const UnfoldedIdentifierExpr *element) {
    _in_use[element->offset()] = true;
}

void PlaceUseVisitor::accept(const UnfoldedUpperBoundsCondition *element) {
    for (auto &p : element->places())
        _in_use[p._place] = true;
}

void PlaceUseVisitor::accept(const EUCondition *el) {
    (*el)[0]->visit(*this);
    (*el)[1]->visit(*this);
}

void PlaceUseVisitor::accept(const AUCondition *el) {
    (*el)[0]->visit(*this);
    (*el)[1]->visit(*this);
}

void PlaceUseVisitor::accept(const EFCondition *el) { (*el)[0]->visit(*this); }
void PlaceUseVisitor::accept(const EGCondition *el) { (*el)[0]->visit(*this); }
void PlaceUseVisitor::accept(const AGCondition *el) { (*el)[0]->visit(*this); }
void PlaceUseVisitor::accept(const AFCondition *el) { (*el)[0]->visit(*this); }
void PlaceUseVisitor::accept(const EXCondition *el) { (*el)[0]->visit(*this); }
void PlaceUseVisitor::accept(const AXCondition *el) { (*el)[0]->visit(*this); }

// shallow elements, neither of these should exist in a compiled expression
void PlaceUseVisitor::accept(const LiteralExpr *element) {}
void PlaceUseVisitor::accept(const DeadlockCondition *) {}

} // namespace PetriEngine::PQL
