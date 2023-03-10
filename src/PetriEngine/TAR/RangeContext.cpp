/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   RangeContext.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on March 31, 2020, 5:01 PM
 */

#include "PetriEngine/TAR/RangeContext.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Evaluation.h"

namespace PetriEngine {
    using namespace PQL;

    RangeContext::RangeContext(prtable_t& table, MarkVal* base, const PetriNet& net, const uint64_t* uses, MarkVal* marking, const std::vector<bool>& dirty)
    : _ranges(table), _base(base), _net(net), _uses(uses), _marking(marking), _dirty(dirty)
    {
    }

    void RangeContext::handle_compare(const Expr_ptr& left, const Expr_ptr& right, bool strict)
    {
        auto vl = left->getEval();
        auto vr = right->getEval();
        if(right->placeFree())
        {
            _limit = vr + (strict ? 0 : 1);
            _lt = false;
            Visitor::visit(this, left);
        }
        else if(left->placeFree())
        {
            _limit = vl - (strict ? 0 : 1);
            _lt = true;
            Visitor::visit(this, right);
        }
        else
        {
            _lt = false;
            _limit = vr + (strict ? 0 : 1);
            Visitor::visit(this, left);
            _lt = true;
            _limit = vr;
            Visitor::visit(this, right);
        }
    }

    void RangeContext::_accept(const NotCondition* element)
    {
        assert(false);
        throw base_error("Unsupported query type for TAR");
    }


    void RangeContext::_accept(const PetriEngine::PQL::AndCondition* element)
    {
        if (element->isSatisfied()) return;
        EvaluationContext ctx(_marking, &_net);
        prtable_t vect = _ranges;
        prtable_t res;
        size_t priority = std::numeric_limits<size_t>::max();
        size_t sum = 0;
        bool dirty = false;
        assert(!_is_dirty);
        for (auto& e : *element) {
            if (PetriEngine::PQL::evaluateAndSet(e.get(), ctx) == Condition::RFALSE) {
                _ranges = vect;
                Visitor::visit(this, e);
                dirty |= _is_dirty;
                if(_is_dirty)
                {
                    _is_dirty = false;
                    continue;
                }
                if (_ranges.nr_places() < priority) {
                    res = _ranges;
                    priority = _ranges.nr_places();
                    sum = 0;
                    for (auto& place : _ranges.places())
                    {
                        sum += _uses[place];
                        assert(!_dirty[place]);
                    }
                }
                else if (_ranges.nr_places() == priority) {
                    size_t lsum = 0;
                    for (auto& place : _ranges.places())
                    {
                        lsum += _uses[place];
                        assert(!_dirty[place]);
                    }
                    if (lsum > sum) {
                        res = _ranges;
                        priority = _ranges.nr_places();
                        sum = lsum;
                    }
                }
            }
        }
        if(priority != std::numeric_limits<size_t>::max())
        {
            _ranges = res;
        }
        else
        {
            assert(dirty);
            _is_dirty = true;
        }
    }

    void RangeContext::_accept(const OrCondition* element)
    {
        // if(element->isSatisfied()) return;
        for (auto& e : *element) {
            //assert(!e->isSatisfied());
            Visitor::visit(this, e);
            if(_is_dirty)
                return;
        }
    }

    void RangeContext::_accept(const NotEqualCondition* element)
    {
        _is_dirty = true; // TODO improve
    }

    void RangeContext::_accept(const EqualCondition* element)
    {
        _is_dirty = true; // TODO improve
    }

    void RangeContext::_accept(const LessThanCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], true);
    }

    void RangeContext::_accept(const LessThanOrEqualCondition* element)
    {
        handle_compare((*element)[0], (*element)[1], false);
    }

    void RangeContext::_accept(const LiteralExpr* element)
    {
    }


    void RangeContext::_accept(const UnfoldedIdentifierExpr* element)
    {
        if(_dirty[element->offset()])
        {
            _is_dirty = true;
            return;
        }

        auto pl = element->offset();
        if (_lt)
            _ranges.restrict_upper(pl, _limit);
        else //TODO: check if _limit is ever negative or matches uint32_t min/max
            _ranges.restrict_lower(pl, _limit);
        assert(_ranges.lower(pl) <= _base[element->offset()]);
        assert(_ranges.upper(pl) >= _base[element->offset()]);
    }

    void RangeContext::_accept(const PlusExpr* element)
    {
        //auto fdf = std::abs(_limit - element->getEval())/
        //(element->places().size() + element->expressions().size());
        for (auto& p : element->places()) {
            if(_dirty[p.first])
            {
                _is_dirty = true;
                return;
            }
            uint32_t pl = p.first;
            if (_lt)
                _ranges.restrict_upper(pl, _base[p.first]);
            else
                _ranges.restrict_lower(pl, _base[p.first]);
            assert(_ranges.lower(pl) <= _base[p.first]);
            assert(_ranges.upper(pl) >= _base[p.first]);
        }
        for (auto& e : element->expressions()) {
            _limit = e->getEval();
            Visitor::visit(this, e);
            if(_is_dirty) return;
        }
    }

    void RangeContext::_accept(const MultiplyExpr*)
    {
        _is_dirty = true; // TODO improve
    }

    void RangeContext::_accept(const MinusExpr*)
    {
        _is_dirty = true; // TODO improve
    }

    void RangeContext::_accept(const SubtractExpr*)
    {
        _is_dirty = true; // TODO improve
    }


    void RangeContext::_accept(const DeadlockCondition* element)
    {
        assert(!element->isSatisfied());
        uint64_t priority = 0;
        size_t cand = 0;
        bool dirty = false;
        for (size_t t = 0; t < _net.numberOfTransitions(); ++t) {
            auto pre = _net.preset(t);
            bool ok = true;
            for (; pre.first != pre.second; ++pre.first) {
                assert(!pre.first->inhibitor);
                if (_base[pre.first->place] < pre.first->tokens) {
                    ok = false;
                }
                if(_dirty[pre.first->place])
                {
                    dirty = true;
                    ok = false;
                }
            }
            if (!ok) continue;
            pre = _net.preset(t);
            uint64_t sum = 0;
            for (; pre.first != pre.second; ++pre.first) {
                sum += _uses[pre.first->place];
                assert(!_dirty[pre.first->place]);
            }
            if (sum > priority) {
                priority = sum;
                cand = t;
            }
        }

        if (priority != 0) {
            auto pre = _net.preset(cand);
            for (; pre.first != pre.second; ++pre.first) {
                uint32_t pl = pre.first->place;
                _ranges.restrict_lower(pl, pre.first->tokens);
                assert(_ranges.lower(pl) <= _base[pl]);
                assert(_ranges.upper(pl) >= _base[pl]);
            }
            return;
        }
        if(dirty)
        {
            _is_dirty = true;
            return;
        }

        assert(false);
    }

    void RangeContext::_accept(const CompareConjunction* element)
    {
        assert(!element->isSatisfied());
        bool disjunction = element->isNegated();
        placerange_t pr;
        uint64_t priority = 0;
        for (auto& c : element->constraints()) {
            if (!disjunction) {
                if (c._lower > _base[c._place]) {
                    if (_ranges.upper(c._place) <= c._lower - 1)
                        return;
                    if (!_dirty[c._place] && priority < _uses[c._place]) {
                        pr = placerange_t();
                        pr._place = c._place;
                        pr._range._upper = c._lower - 1;
                        priority = _uses[c._place];
                        assert(pr._range._lower <= _base[c._place]);
                        assert(pr._range._upper >= _base[c._place]);
                    }
                }
                else if (c._upper < _base[c._place]) {
                    _ranges.add_place(c._place); //make sure this place exists

                    if (_ranges.lower(c._place) >= c._upper + 1)
                        return;
                    if (!_dirty[c._place] && priority < _uses[c._place]) {
                        pr = placerange_t();
                        priority = _uses[c._place];
                        pr._place = c._place;
                        pr._range._lower = c._upper + 1;
                        pr._place = c._place;
                        assert(pr._range._lower <= _base[c._place]);
                        assert(pr._range._upper >= _base[c._place]);
                    }
                }
            }
            else {
                _ranges.add_place(c._place); // TODO: Check if this is actually a restriction
                _ranges.set_lower(c._place, std::max(c._lower, pr._range._lower));
                _ranges.set_upper(c._place, std::min(c._upper, pr._range._upper));
                assert(_ranges.lower(c._place) <= _base[c._place]);
                assert(_ranges.upper(c._place) >= _base[c._place]);
                if(_dirty[c._place])
                {
                    _is_dirty = true;
                    return;
                }
            }
        }
        if (!disjunction) {
            if(priority == 0)
            {
                _is_dirty = true;
                return;
            }
            assert(priority > 0);
            _ranges.set_place(pr._place, pr._range._lower, pr._range._upper);
            _ranges.compress();
        }
    }

    void RangeContext::_accept(const UnfoldedUpperBoundsCondition* element)
    {
        for(auto& pb : element->places())
        {
            //TMGR: Why double?
            _ranges.set_upper(pb._place, std::min<double>(pb._max, _marking[pb._place]));
        }
    }
}
