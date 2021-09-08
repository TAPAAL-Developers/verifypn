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

#include "PetriEngine/Stubborn/InterestingTransitionVisitor.h"

namespace PetriEngine {

void InterestingTransitionVisitor::accept(const PQL::SimpleQuantifierCondition *element) {
    (*element)[0]->visit(*this);
}

void InterestingTransitionVisitor::accept(const PQL::UntilCondition *element) {
    element->get_cond1()->visit(*this);
    negate();
    element->get_cond1()->visit(*this);
    negate();
    element->get_cond2()->visit(*this);
}

void InterestingTransitionVisitor::accept(const PQL::AndCondition *element) {
    if (!_negated) { // and
        for (auto &c : *element) {
            if (!c->is_satisfied()) {
                c->visit(*this);
                break;
            }
        }
    } else { // or
        for (auto &c : *element)
            c->visit(*this);
    }
}

void InterestingTransitionVisitor::accept(const PQL::OrCondition *element) {
    if (!_negated) { // or
        for (auto &c : *element)
            c->visit(*this);
    } else { // and
        for (auto &c : *element) {
            if (c->is_satisfied()) {
                c->visit(*this);
                break;
            }
        }
    }
}

void InterestingTransitionVisitor::accept(const PQL::CompareConjunction *element) {

    auto neg = _negated != element->is_negated();
    int32_t cand = std::numeric_limits<int32_t>::max();
    bool pre = false;
    for (auto &c : *element) {
        auto val = _stubborn.get_parent()[c._place];
        if (c._lower == c._upper) {
            if (neg) {
                if (val != c._lower)
                    continue;
                _stubborn.postset_of(c._place, _closure);
                _stubborn.preset_of(c._place, _closure);
            } else {
                if (val == c._lower)
                    continue;
                if (val > c._lower) {
                    cand = c._place;
                    pre = false;
                } else {
                    cand = c._place;
                    pre = true;
                }
            }
        } else {
            if (!neg) {
                if (val < c._lower && c._lower != 0) {
                    assert(!neg);
                    cand = c._place;
                    pre = true;
                }

                if (val > c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                    assert(!neg);
                    cand = c._place;
                    pre = false;
                }
            } else {
                if (val >= c._lower && c._lower != 0) {
                    _stubborn.postset_of(c._place, _closure);
                }

                if (val <= c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                    _stubborn.preset_of(c._place, _closure);
                }
            }
        }
        if (cand != std::numeric_limits<int32_t>::max()) {
            if (pre && _stubborn.seen_pre(cand))
                return;
            else if (!pre && _stubborn.seen_post(cand))
                return;
        }
    }
    if (cand != std::numeric_limits<int32_t>::max()) {
        if (pre) {
            _stubborn.preset_of(cand, _closure);
        } else if (!pre) {
            _stubborn.postset_of(cand, _closure);
        }
    }
}

void InterestingTransitionVisitor::accept(const PQL::EqualCondition *element) {
    if (!_negated) { // equal
        if ((*element)[0]->get_eval() == (*element)[1]->get_eval()) {
            return;
        }
        if ((*element)[0]->get_eval() > (*element)[1]->get_eval()) {
            (*element)[0]->visit(_decr);
            (*element)[1]->visit(_incr);
        } else {
            (*element)[0]->visit(_incr);
            (*element)[1]->visit(_decr);
        }
    } else { // not equal
        if ((*element)[0]->get_eval() != (*element)[1]->get_eval()) {
            return;
        }
        (*element)[0]->visit(_incr);
        (*element)[0]->visit(_decr);
        (*element)[1]->visit(_incr);
        (*element)[1]->visit(_decr);
    }
}

void InterestingTransitionVisitor::accept(const PQL::NotEqualCondition *element) {
    if (!_negated) { // not equal
        if ((*element)[0]->get_eval() != (*element)[1]->get_eval()) {
            return;
        }
        (*element)[0]->visit(_incr);
        (*element)[0]->visit(_decr);
        (*element)[1]->visit(_incr);
        (*element)[1]->visit(_decr);
    } else { // equal
        if ((*element)[0]->get_eval() == (*element)[1]->get_eval()) {
            return;
        }
        if ((*element)[0]->get_eval() > (*element)[1]->get_eval()) {
            (*element)[0]->visit(_decr);
            (*element)[1]->visit(_incr);
        } else {
            (*element)[0]->visit(_incr);
            (*element)[1]->visit(_decr);
        }
    }
}

void InterestingTransitionVisitor::accept(const PQL::LessThanCondition *element) {
    if (!_negated) { // less than
        if ((*element)[0]->get_eval() < (*element)[1]->get_eval()) {
            return;
        }
        (*element)[0]->visit(_decr);
        (*element)[1]->visit(_incr);
    } else { // greater than or equal
        if ((*element)[0]->get_eval() >= (*element)[1]->get_eval()) {
            return;
        }
        (*element)[0]->visit(_incr);
        (*element)[1]->visit(_decr);
    }
}

void InterestingTransitionVisitor::accept(const PQL::LessThanOrEqualCondition *element) {
    if (!_negated) { // less than or equal
        if ((*element)[0]->get_eval() <= (*element)[1]->get_eval()) {
            return;
        }
        (*element)[0]->visit(_decr);
        (*element)[1]->visit(_incr);
    } else { // greater than
        if ((*element)[0]->get_eval() > (*element)[1]->get_eval()) {
            return;
        }
        (*element)[0]->visit(_incr);
        (*element)[1]->visit(_decr);
    }
}

void InterestingTransitionVisitor::accept(const PQL::NotCondition *element) {
    negate();
    (*element)[0]->visit(*this);
    negate();
}

void InterestingTransitionVisitor::accept(const PQL::BooleanCondition *element) {
    // Add nothing
}

void InterestingTransitionVisitor::accept(const PQL::DeadlockCondition *element) {
    if (!element->is_satisfied()) {
        _stubborn.post_preset_of(_stubborn.least_dependent_enabled(), _closure);
    } // else add nothing
}

void InterestingTransitionVisitor::accept(const PQL::UnfoldedUpperBoundsCondition *element) {
    for (auto &p : element->places())
        if (!p._maxed_out)
            _stubborn.preset_of(p._place);
}

void InterestingTransitionVisitor::accept(const PQL::GCondition *element) {
    negate();
    (*element)[0]->visit(*this);
    negate();
}

void InterestingTransitionVisitor::accept(const PQL::FCondition *element) {
    (*element)[0]->visit(*this);
}

void InterestingTransitionVisitor::accept(const PQL::EFCondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::EGCondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::AGCondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::AFCondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::EXCondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::AXCondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::ACondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::ECondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::XCondition *condition) {
    accept(static_cast<const PQL::SimpleQuantifierCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::AUCondition *condition) {
    accept(static_cast<const PQL::UntilCondition *>(condition));
}

void InterestingTransitionVisitor::accept(const PQL::EUCondition *condition) {
    accept(static_cast<const PQL::UntilCondition *>(condition));
}

void InterestingTransitionVisitor::IncrVisitor::accept(const PQL::PlusExpr *element) {
    for (auto &i : element->places())
        _stubborn.preset_of(i.first, _closure);
    for (auto &e : element->expressions())
        e->visit(*this);
}

void InterestingTransitionVisitor::DecrVisitor::accept(const PQL::PlusExpr *element) {
    for (auto &i : element->places())
        _stubborn.postset_of(i.first, _closure);
    for (auto &e : element->expressions())
        e->visit(*this);
}

void InterestingTransitionVisitor::IncrVisitor::accept(const PQL::SubtractExpr *element) {
    bool first = true;
    for (auto &e : element->expressions()) {
        if (first)
            e->visit(*this);
        else
            e->visit(*_decr);
        first = false;
    }
}

void InterestingTransitionVisitor::DecrVisitor::accept(const PQL::SubtractExpr *element) {
    bool first = true;
    for (auto &e : element->expressions()) {
        if (first)
            e->visit(*this);
        else
            e->visit(*_incr);
        first = false;
    }
}

void InterestingTransitionVisitor::IncrVisitor::accept(const PQL::MultiplyExpr *element) {
    if ((element->places().size() + element->expressions().size()) == 1) {
        for (auto &i : element->places())
            _stubborn.preset_of(i.first, _closure);
        for (auto &e : element->expressions())
            e->visit(*this);
    } else {
        for (auto &i : element->places()) {
            _stubborn.preset_of(i.first, _closure);
            _stubborn.postset_of(i.first, _closure);
        }
        for (auto &e : element->expressions()) {
            e->visit(*this);
            e->visit(*_decr);
        }
    }
}

void InterestingTransitionVisitor::DecrVisitor::accept(const PQL::MultiplyExpr *element) {
    if ((element->places().size() + element->expressions().size()) == 1) {
        for (auto &i : element->places())
            _stubborn.postset_of(i.first, _closure);
        for (auto &e : element->expressions())
            e->visit(*this);
    } else
        element->visit(*_incr);
}

void InterestingTransitionVisitor::IncrVisitor::accept(const PQL::MinusExpr *element) {
    // TODO not implemented
}

void InterestingTransitionVisitor::DecrVisitor::accept(const PQL::MinusExpr *element) {
    // TODO not implemented
}

void InterestingTransitionVisitor::IncrVisitor::accept(const PQL::LiteralExpr *element) {
    // Add nothing
}

void InterestingTransitionVisitor::DecrVisitor::accept(const PQL::LiteralExpr *element) {
    // Add nothing
}

void InterestingTransitionVisitor::IncrVisitor::accept(
    const PQL::UnfoldedIdentifierExpr *element) {
    _stubborn.preset_of(element->offset(), _closure);
}

void InterestingTransitionVisitor::DecrVisitor::accept(
    const PQL::UnfoldedIdentifierExpr *element) {
    _stubborn.postset_of(element->offset(), _closure);
}

void InterestingLTLTransitionVisitor::accept(const PQL::LessThanCondition *element) {
    negate_if_satisfied<PQL::LessThanCondition>(element);
}

void InterestingLTLTransitionVisitor::accept(const PQL::LessThanOrEqualCondition *element) {
    negate_if_satisfied<PQL::LessThanOrEqualCondition>(element);
}

void InterestingLTLTransitionVisitor::accept(const PQL::EqualCondition *element) {
    negate_if_satisfied<PQL::EqualCondition>(element);
}

void InterestingLTLTransitionVisitor::accept(const PQL::NotEqualCondition *element) {
    negate_if_satisfied<PQL::NotEqualCondition>(element);
}

void InterestingLTLTransitionVisitor::accept(const PQL::CompareConjunction *element) {
    auto neg = _negated != element->is_negated();
    for (auto &c : *element) {
        if (!neg) {
            if (c._lower != 0 && !_stubborn.seen_pre(c._place)) {
                // c < p becomes satisfied by preset of p.
                _stubborn.preset_of(c._place, _closure);
            }
            if (c._upper != std::numeric_limits<uint32_t>::max() &&
                !_stubborn.seen_post(c._place)) {
                // p < c becomes satisfied by postset of p.
                _stubborn.postset_of(c._place, _closure);
            }
        } else {
            if (c._lower != 0 && !_stubborn.seen_post(c._place)) {
                // !(p < c) becomes satisfied by preset of p.
                _stubborn.postset_of(c._place, _closure);
            }
            if (c._upper != std::numeric_limits<uint32_t>::max() && !_stubborn.seen_pre(c._place)) {
                // !(c < p) becomes satisfied by postset of p.
                _stubborn.preset_of(c._place, _closure);
            }
        }
    }
}

template <typename Condition>
void InterestingLTLTransitionVisitor::negate_if_satisfied(const Condition *element) {
    auto isSatisfied = element->get_satisfied();
    assert(isSatisfied != PQL::Condition::RUNKNOWN);
    if ((isSatisfied == PQL::Condition::RTRUE) != _negated) {
        negate();
        InterestingTransitionVisitor::accept(element);
        negate();
    } else
        InterestingTransitionVisitor::accept(element);
}

void AutomatonInterestingTransitionVisitor::accept(const PQL::CompareConjunction *element) {
    auto neg = _negated != element->is_negated();
    for (auto &c : *element) {
        int32_t cand = std::numeric_limits<int32_t>::max();
        bool pre = false;
        auto val = _stubborn.get_parent()[c._place];
        if (c._lower == c._upper) {
            if (neg) {
                if (val != c._lower)
                    continue;
                _stubborn.postset_of(c._place, _closure);
                _stubborn.preset_of(c._place, _closure);
            } else {
                if (val == c._lower)
                    continue;
                if (val > c._lower) {
                    cand = c._place;
                    pre = false;
                } else {
                    cand = c._place;
                    pre = true;
                }
            }
        } else {
            if (!neg) {
                if (val < c._lower && c._lower != 0) {
                    assert(!neg);
                    cand = c._place;
                    pre = true;
                }

                if (val > c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                    assert(!neg);
                    cand = c._place;
                    pre = false;
                }
            } else {
                if (val >= c._lower && c._lower != 0) {
                    _stubborn.postset_of(c._place, _closure);
                }

                if (val <= c._upper && c._upper != std::numeric_limits<uint32_t>::max()) {
                    _stubborn.preset_of(c._place, _closure);
                }
            }
        }
        if (cand != std::numeric_limits<int32_t>::max()) {
            if (pre) {
                _stubborn.preset_of(cand, _closure);
            } else if (!pre) {
                _stubborn.postset_of(cand, _closure);
            }
            cand = std::numeric_limits<int32_t>::max();
        }
    }
}
} // namespace PetriEngine