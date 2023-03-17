/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   range.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on March 31, 2020, 4:32 PM
 */

#ifndef RANGE_H
#define RANGE_H

#include <pardibaal/DBM.h>
#include <cinttypes>
#include <cassert>
#include <limits>
#include <iostream>
#include <vector>
#include <cstdlib>

namespace PetriEngine {
    namespace Reachability {
        using bound_t = pardibaal::bound_t;
        using diff_bound_t = pardibaal::difference_bound_t;

        struct range_t {

            static inline uint32_t min() {
                return std::numeric_limits<uint32_t>::min();
            }

            static inline uint32_t max() {
                return std::numeric_limits<uint32_t>::max();
            }

            range_t() {
            };

            explicit range_t(uint32_t val) : _lower(val), _upper(val) {
            }

            range_t(uint32_t l, uint32_t u) : _lower(l), _upper(u) {
                assert(_lower <= _upper);
            }

            uint32_t _lower = min();
            uint32_t _upper = max();

            bool no_upper() const {
                return _upper == max();
            }

            bool no_lower() const {
                return _lower == min();
            }

            bool unbound() const {
                return no_lower() && no_upper();
            }

            bool isSound() const {
                return _lower <= _upper;
            }

            bool contains(uint32_t id) const {
                return _lower <= id && id <= _upper;
            }

            void free() {
                _upper = max();
                _lower = min();
            }

            uint32_t size() const {
                return 1 + _upper - _lower;
            }

            void invalidate() {
                //hack setting range invalid
                _lower = 1;
                _upper = 0;
            }

            std::ostream& print(std::ostream& os) const {
                if (no_lower())
                    os << "[0";
                else
                    os << "[" << _lower;
                os << ", ";
                if (no_upper())
                    os << "inf]";
                else
                    os << _upper << "]";
                return os;
            }

            std::pair<bool, bool> compare(const range_t& other) const {
                return std::make_pair(
                        _lower <= other._lower && _upper >= other._upper,
                        _lower >= other._lower && _upper <= other._upper
                        );
            }

            bool intersects(const range_t& other) const {
                return _lower <= other._upper  && other._lower <= _upper;
            }
            
            range_t& operator&=(const range_t& other) {
                _lower = std::max(_lower, other._lower);
                _upper = std::min(_upper, other._upper);
                return *this;
            }

            range_t& operator|=(const range_t& other) {
                _lower = std::min(_lower, other._lower);
                _upper = std::max(_upper, other._upper);
                return *this;
            }

            range_t& operator|=(uint32_t val) {
                _lower = std::min(val, _lower);
                _upper = std::max(val, _upper);
                return *this;
            }

            range_t& operator&=(uint32_t val) {
                _lower = val;
                _upper = val;
                return *this;
            }

            range_t& operator-=(uint32_t val) {
                if (_lower < min() + val)
                    _lower = min();
                else
                    _lower -= val;
                if (_upper != max()) {
                    if (_upper < min() + val) {
                        assert(false);
                    } else {
                        _upper -= val;
                    }
                }
                return *this;
            }

            range_t& operator+=(uint32_t val) {
                if (_lower != min()) {
                    if (_lower >= max() - val)
                        assert(false);
                    _lower += val;
                }

                if (_upper >= max() - val) {
                    _upper = max();
                } else
                    _upper += val;
                return *this;
            }
        };

        struct placerange_t {
            range_t _range;
            uint32_t _place = std::numeric_limits<uint32_t>::max();

            placerange_t() {
            }

            placerange_t(uint32_t place) : _place(place) {
            };

            placerange_t(uint32_t place, const range_t& r) : _range(r), _place(place) {
            };

            placerange_t(uint32_t place, range_t&& r) : _range(r), _place(place) {
            };

            placerange_t(uint32_t place, uint32_t v) : _range(v), _place(place) {
            };

            placerange_t(uint32_t place, uint32_t l, uint32_t u) : _range(l, u), _place(place) {
            };

            std::ostream& print(std::ostream& os) const {
                os << "<P" << _place << "> in ";
                return _range.print(os);
            }

            std::pair<bool, bool> compare(const range_t& other) const {
                return _range.compare(other);
            }

            std::pair<bool, bool> compare(const placerange_t& other) const {
                assert(other._place == _place);
                if (other._place != _place)
                    return std::make_pair(false, false);
                return _range.compare(other._range);
            }

            placerange_t& operator|=(uint32_t val) {
                _range |= val;
                return *this;
            }

            placerange_t& operator&=(uint32_t val) {
                _range &= val;
                return *this;
            }

            placerange_t& operator-=(uint32_t val) {
                _range -= val;
                return *this;
            }

            placerange_t& operator+=(uint32_t val) {
                _range += val;
                return *this;
            }

            placerange_t& operator&=(const placerange_t& other) {
                assert(other._place == _place);
                _range &= other._range;
                return *this;
            }

            placerange_t& operator|=(const placerange_t& other) {
                assert(other._place == _place);
                _range |= other._range;
                return *this;
            }

            // used for sorting only!

            bool operator<(const placerange_t& other) const {
                return _place < other._place;
            }
        };

        struct prtable_t {
            /*
             * Operations comparing bounds in this and input:
             * Restrict (&=) Set the bound to the most strict of the two
             * Relax (|=) relaxing a bound to be the more relaxed of the two
             * Add (+=) increases a bound
             * Subtract (-=) lowers a bound
             */

            bool no_lower(uint32_t place) const {
                if (!contains_place(place))
                    return true;
                return _dbm.at(0, _placemapping[place]).get_bound() == 0;
            }

            bool no_upper(uint32_t place) const {
                if (!contains_place(place))
                    return true;
                return _dbm.at(_placemapping[place], 0).is_inf();
            }

            uint32_t lower(uint32_t place) const {
                assert(contains_place(place));
                auto bound = _dbm.at(0, _placemapping[place]);
                
                // if lower bound is inf, then the zone should be empty
                return bound.is_inf() ? std::numeric_limits<uint32_t>::max() :  (uint32_t) std::abs(bound.get_bound());
            }

            uint32_t upper(uint32_t place) const {
                assert(contains_place(place));
                auto bound = _dbm.at(_placemapping[place], 0);

                return bound.is_inf() ? std::numeric_limits<uint32_t>::max() : bound.get_bound();
            }

            // Represents the bound place <= n 
            bound_t upper_bound(uint32_t place) const {
                 assert(contains_place(place));
                return _dbm.at(_placemapping[place], 0);
            }

            // Represents the bound 0 - place <= n Note that the bound value can be negative
            bound_t lower_bound(uint32_t place) const {
                 assert(contains_place(place));
                return _dbm.at(0, _placemapping[place]);
            }

            bound_t difference(uint32_t p, uint32_t q) const {
                assert(contains_place(p) && contains_place(q));
                return _dbm.at(_placemapping[p], _placemapping[q]);
            }

            bool is_unbound(uint32_t place) const {
                if (!contains_place(place))
                    return true;
                return _dbm.at(_placemapping[place], 0).is_inf() && 
                       _dbm.at(0, _placemapping[place]).get_bound() == 0;
            }

            void compress() {
                for (uint32_t place = 0; place < _placemapping.size(); ++place)
                    if (contains_place(place) && is_unbound(place))
                        remove_place(place);
                assert(is_compact());
            }

            bool is_compact() const {
                for (uint32_t place = 0; place < _placemapping.size(); ++place)
                    if (contains_place(place) && is_unbound(place))
                        return false;
                return true;
            }

            bool is_true() const {
                return _dbm.dimension() == 1;
            }

            bool is_false(size_t nplaces) const {
                for (uint32_t p = 0; p < _placemapping.size(); ++p) {
                    if (!contains_place(p)) return false; // All places should be indexed
                    if (lower(p) != 0 || upper(p) != 0) // All upper/lower bounds should be 0
                        return false;
                }
                return true;
            }

            std::pair<bool, bool> compare(const prtable_t& other) const {
                auto cmp = std::make_pair(true, true);

                for (uint32_t p = 0; p < _placemapping.size(); ++p) {
                    if (!cmp.first && !cmp.second) break;

                    if (contains_place(p) && other.contains_place(p)) {
                        cmp.first = cmp.first && this->lower(p) <= other.lower(p) && this->upper(p) >= other.upper(p);
                        cmp.second = cmp.second && this->lower(p) >= other.lower(p) && this->upper(p) <= other.upper(p);
                    }

                    for (uint32_t q = 0; q < _placemapping.size(); ++q) {
                        if (!cmp.first && !cmp.second) break;

                        if (cmp.first && ((!contains_place(p) && other.contains_place(p)) || (!contains_place(q) && other.contains_place(q))))
                            cmp.first = false;
                        if (cmp.second && ((contains_place(p) && !other.contains_place(p)) || (contains_place(q) && !other.contains_place(q))))
                            cmp.second = false;
                        
                        if (contains_place(p) && contains_place(q) && other.contains_place(p) && other.contains_place(q)) {
                            cmp.first = cmp.first && this->difference(p, q) >= other.difference(p, q);
                            cmp.second = cmp.second && this->difference(p, q) <= other.difference(p, q);
                        }
                    }
                }
                return cmp;
            }
            
            void restrict_lower(uint32_t place, uint32_t lower) {
                assert(lower != std::numeric_limits<uint32_t>::max());
                // if (lower == std::numeric_limits<uint32_t>::max())
                    // restrict_lower(_placemapping[place], bound_t::inf());
                // else
                if (!contains_place(place))
                    add_place(place);
                _dbm.restrict(0, _placemapping[place], bound_t::non_strict(-1 * (int32_t)lower));
            }

            void restrict_upper(uint32_t place, uint32_t upper) {
                if (!contains_place(place))
                    add_place(place);

                if (upper == std::numeric_limits<uint32_t>::max())
                    _dbm.restrict(_placemapping[place], 0, bound_t::inf());
                else
                    _dbm.restrict(_placemapping[place], 0, bound_t::non_strict(upper));
            }

            void restrict_place(uint32_t place, uint32_t lower, uint32_t upper) {
                restrict_lower(place, lower);
                restrict_upper(place, upper);
            }

            void set_lower(uint32_t place, uint32_t lower) {
                assert(lower != std::numeric_limits<uint32_t>::max());
                if (!contains_place(place)) // if place does not exist, then add it and do restriction
                    restrict_lower(place, lower);
                else
                    set_lower(_placemapping[place], bound_t::non_strict(-1 * lower));

            }

            void set_upper(uint32_t place, uint32_t upper) {
                if (!contains_place(place))
                    restrict_upper(place, upper);
                else {
                    if (upper == std::numeric_limits<uint32_t>::max())
                        set_upper(_placemapping[place], bound_t::inf());
                    else
                        set_upper(_placemapping[place], bound_t::non_strict(upper));
                }
            }

            void set_place(uint32_t place, uint32_t lower, uint32_t upper) {
                set_lower(place, lower);
                set_upper(place, upper);
            }

            void shift_lower(uint32_t place, int32_t value) {
                if (!contains_place(place))
                    add_place(place);

                if (lower(place) > 0) {
                    assert(!(value > 0 && lower(place) > std::numeric_limits<uint32_t>::max() - value));
                    if (value < 0 && lower(place) < value)
                        set_lower(place, 0);
                    else
                        set_lower(place, lower(place) + value);
                }
            }

            void shift_upper(uint32_t place, int32_t value) {
                if (!contains_place(place))
                    add_place(place);
                
                if (upper(place) < std::numeric_limits<uint32_t>::max()) {
                    if (value > 0 && upper(place) < std::numeric_limits<uint32_t>::max() - value)
                        set_upper(place, std::numeric_limits<uint32_t>::max());
                    else if (value < 0 && upper(place) < value)
                        set_upper(place, 0);
                    else
                        set_upper(place, upper(place) + value);
                }
            }

            void shift_place(uint32_t place, int32_t value) {               
                shift_upper(place, value);
                shift_lower(place, value);
                // _dbm.shift(_placemapping[place], value);
            }

            prtable_t& operator&=(const placerange_t& range) {
                restrict_place(range._place, range._range._lower, range._range._upper);
            }
            
            bool restricts(const std::vector<int64_t> writes) const {
                for (auto p : writes) {
                    if (contains_place(p) && !is_unbound(p))
                        return true;
                }
                return false;
            }

            bool operator<(const prtable_t& other) const {
                if (this->_dbm.dimension() != other._dbm.dimension())
                    return _dbm.dimension() < other._dbm.dimension();
                for (uint32_t i = 0; i < _placemapping.size(); ++i) {
                    if (this->contains_place(i) &&  other.contains_place(i)) {
                        if (this->lower(i) != other.lower(i))
                            return this->lower(i) < other.lower(i);
                        if (this->upper(i) != other.upper(i))
                            return this->upper(i) < other.upper(i);
                    } else if (this->contains_place(i) || other.contains_place(i))
                        return this->contains_place(i);

                }
                return false;
            }

            bool operator==(const prtable_t& other) const {
                auto r = compare(other);
                return r.first && r.second;
            }
            
            std::ostream& print_lower_upper(std::ostream& os) const {

                os << "{\n";
                for (uint32_t i = 0; i < _placemapping.size(); ++i) {
                    if (contains_place(i)) {
                        os << "\t<P" << i << "> in [" << lower(i) << ", ";
                        if (_dbm.at(_placemapping[i], 0).is_inf())
                            os << "inf]";
                        else
                            os << upper(i) << "]";
                        os << "\n";
                    }
                }
                os << "}\n";
                return os;
            }

            std::ostream& print(std::ostream& os) const {
                os << "Place: ";
                for (uint32_t i = 0; i < _placemapping.size(); ++i)
                    if (contains_place(i))
                        os << i << " ";

                os << "\nIndex: ";
                for (uint32_t i = 0; i < _placemapping.size(); ++i)
                    if (contains_place(i))
                        os << _placemapping[i] << " ";

                os << _dbm;
                return os;
            }

            void copy(const prtable_t& other) {
                _placemapping = other._placemapping;
                _dbm = pardibaal::DBM(other._dbm);
            }

            // prtable_t& operator&=(const prtable_t& other) {
            //     for (uint32_t i = 0; i < _placemapping.size(); ++i) {
            //         // Go through only places containted in other
            //         if (other.contains_place(i)) { //Restriction automatically adds places if not contained
            //             this->restrict_place(i, other.lower(i), other.upper(i));
                        
            //             // Restrict all diagonal bounds from other in this
            //             for (uint32_t j = 0; j < _placemapping.size(); ++j)
            //                 if (other.contains_place(j))
            //                     this->restrict_difference(i, j, other.difference(i, j));
            //         }
            //     }
            //     return *this;
            // }

            uint32_t nr_places() const {
                return _dbm.dimension() - 1;
            }

            std::vector<uint32_t> places() const {
                std::vector<uint32_t> ret = std::vector<uint32_t>(nr_places());
                int r = 0;

                for (uint32_t i = 0; i < _placemapping.size(); ++i) {
                    if (contains_place(i))
                        ret[r++] = i;
                }
                assert(r == ret.size());
                return ret;
            }

            bool inline contains_place(uint32_t place) const {
                return _placemapping[place] != 0; 
            }

            // Add unbounded place if not already contained
            void add_place(uint32_t place) {
                if (!contains_place(place)) {
                    _placemapping[place] = _dbm.dimension();
                    _dbm.add_clock_at(_dbm.dimension());
                    _dbm.free(_dbm.dimension() - 1);
                }
            }

        private:
            // Placemapping to map places to indexes in the dbm. The size is constant and equal to number of places
            std::vector<uint32_t> _placemapping{};
            pardibaal::DBM _dbm = pardibaal::DBM(1);

            bool inline contains_index(uint32_t index) const { return index < _dbm.dimension(); }

            void remove_place(uint32_t place) {
                assert(contains_place(place));
                _dbm.remove_clock(_placemapping[place]);

                // When we remove an index in the dbm, indexes above this are shifted one down.
                for (uint32_t i = 0; i < _placemapping.size(); ++i)
                    if (_placemapping[i] > _placemapping[place])
                        --(_placemapping[i]);

                _placemapping[place] = 0;
            }

            void remove_index(uint32_t index) {
                bool found = false;
                for (uint32_t i = 0; i < _placemapping.size(); ++i) {
                    if (_placemapping[i] = index) {
                        found = true;
                        _placemapping[i] = 0;
                        break;
                    } else if (_placemapping[i] > index)
                        --(_placemapping[i]); // shift mapping down to compensate removal
                }
                assert(found);
                _dbm.remove_clock(index);
            }

            bool unbound_index(uint32_t index) const {
                if (!contains_index(index))
                    return true;
                return _dbm.at(index, 0).is_inf() && 
                       _dbm.at(0, index).get_bound() == 0;
            }

            void set_lower(uint32_t index, bound_t bound) {
                assert(!bound.is_inf()); // Lower bounds should never be inf (at most 0)
                
                if (_dbm.at(0, index) == bound)
                    return;

                if (_dbm.at(0, index) > bound)
                    _dbm.restrict(0, index, bound);
                else {
                    int32_t diff = bound.get_bound() - _dbm.at(0, index).get_bound();
                    for (uint32_t i = 1; i < _dbm.dimension(); ++i) {
                        if (i == index) continue; // Add the difference to all lower diagonal bounds
                        _dbm.set(i, index, _dbm.at(i, index) + diff);
                    }
                }
            }

            void set_upper(uint32_t index, bound_t bound) {
                if (_dbm.at(index, 0) == bound)
                    return;

                if (_dbm.at(index, 0) > bound)
                    _dbm.restrict(index, 0, bound);
                else {
                    int32_t diff = bound.get_bound() - _dbm.at(index, 0).get_bound();
                    for (uint32_t i = 1; i < _dbm.dimension(); ++i) {
                        if (i == index) continue;
                        if (bound.is_inf())
                            _dbm.set(index, i, bound_t::inf());
                        else
                            _dbm.set(index, i, _dbm.at(index, i) + diff);
                    }
                }
            }
        };

        struct prvector_t {
            std::vector<placerange_t> _ranges;

            const placerange_t* operator[](uint32_t place) const {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if (lb == _ranges.end() || lb->_place != place) {
                    return nullptr;
                } else {
                    return &(*lb);
                }
            }

            placerange_t* operator[](uint32_t place) {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if (lb == _ranges.end() || lb->_place != place) {
                    return nullptr;
                } else {
                    return &(*lb);
                }
            }

            placerange_t& find_or_add(uint32_t place) {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
                if (lb == _ranges.end() || lb->_place != place) {
                    lb = _ranges.emplace(lb, place);
                }
                return *lb;
            }

            uint32_t lower(uint32_t place) const {
                auto* pr = (*this)[place];
                if (pr == nullptr)
                    return range_t::min();
                return pr->_range._lower;
            }

            uint32_t upper(uint32_t place) const {
                auto* pr = (*this)[place];
                if (pr == nullptr)
                    return range_t::max();
                return pr->_range._upper;
            }

            bool unbound(uint32_t place) const {
                auto* pr = (*this)[place];
                if (pr == nullptr)
                    return true;
                return pr->_range.unbound();
            }

            void copy(const prvector_t& other) {
                _ranges = other._ranges;
                compact();
            }

            // Removes all bounds that are unbounded
            void compact() {
                for (int64_t i = _ranges.size() - 1; i >= 0; --i) {
                    if (_ranges[i]._range.unbound())
                        _ranges.erase(_ranges.begin() + i);
                }
                assert(is_compact());
            }

            // True if all bounds are bounded
            bool is_compact() const {
                for (auto& e : _ranges)
                    if (e._range.unbound())
                        return false;
                return true;
            }

            std::pair<bool, bool> compare(const prvector_t& other) const {
                assert(is_compact());
                assert(other.is_compact());
                auto sit = _ranges.begin();
                auto oit = other._ranges.begin();
                std::pair<bool, bool> incl = std::make_pair(true, true);

                while (true) {
                    if (sit == _ranges.end()) {
                        incl.second = incl.second && (oit == other._ranges.end());
                        break;
                    } else if (oit == other._ranges.end()) {
                        incl.first = false;
                        break;
                    } else if (sit->_place == oit->_place) {
                        auto r = sit->compare(*oit);
                        incl.first = incl.first && r.first;
                        incl.second &= incl.second && r.second;
                        ++sit;
                        ++oit;
                    } else if (sit->_place < oit->_place) {
                        incl.first = false;
                        ++sit;
                    } else if (sit->_place > oit->_place) {
                        incl.second = false;
                        ++oit;
                    }
                    if (!incl.first && !incl.second)
                        return incl;
                }
                return incl;
            }

            std::ostream& print(std::ostream& os) const {

                os << "{\n";
                for (auto& pr : _ranges) {
                    os << "\t";
                    pr.print(os) << "\n";
                }
                os << "}\n";
                return os;
            }

            bool is_true() const {
                return _ranges.empty();
            }

            bool is_false(size_t nplaces) const {
                if (_ranges.size() != nplaces) return false;
                for (auto& p : _ranges) {
                    if (p._range._lower != 0 ||
                            p._range._upper != 0)
                        return false;
                }
                return true;
            }

            bool operator<(const prvector_t& other) const {
                if (_ranges.size() != other._ranges.size())
                    return _ranges.size() < other._ranges.size(); // TMGR what if the ranges are not refering to the same places?
                for (size_t i = 0; i < _ranges.size(); ++i) {
                    auto& r = _ranges[i];
                    auto& otr = other._ranges[i];

                    if (r._place != otr._place)
                        return r._place < otr._place;
                    if (r._range._lower != otr._range._lower)
                        return r._range._lower < otr._range._lower;
                    if (r._range._upper != otr._range._upper)
                        return r._range._upper < otr._range._upper;
                }
                return false;
            }

            bool operator==(const prvector_t& other) const {
                auto r = compare(other);
                return r.first && r.second;
            }

            prvector_t& operator&=(const placerange_t& other) {
                auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), other);
                if (lb == std::end(_ranges) || lb->_place != other._place) {
                    _ranges.insert(lb, other);
                } else {
                    *lb &= other;
                }
                return *this;
            }

            prvector_t& operator&=(const prvector_t& other) {
                auto oit = other._ranges.begin();
                auto sit = _ranges.begin();
                while (sit != _ranges.end()) {
                    while (oit != other._ranges.end() && oit->_place < sit->_place) {
                        sit = _ranges.insert(sit, *oit);
                        ++sit;
                        ++oit;
                    }
                    if (oit == other._ranges.end() || oit->_place != sit->_place) {
                        ++sit;
                    } else {
                        *sit &= *oit;
                        ++sit;
                    }
                }
                if (oit != other._ranges.end()) {
                    _ranges.insert(_ranges.end(), oit, other._ranges.end());
                }
                return *this;
            }

            //TMGR: Why int64?? only compared (even assigned) to int32 
            bool restricts(const std::vector<int64_t>& writes) const {
                auto rit = _ranges.begin();
                for (auto p : writes) {
                    while (rit != std::end(_ranges) &&
                            (rit->_place < p || rit->_range.unbound()))
                        ++rit;
                    if (rit == std::end(_ranges)) break;
                    if (rit->_place == p)
                        return true;
                }
                return false;
            }
        };        
    }
}


#endif /* RANGE_H */

