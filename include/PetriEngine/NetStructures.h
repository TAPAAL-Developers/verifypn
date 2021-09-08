/*
 * File:   NetStructures.h
 * Author: Peter G. Jensen
 *
 * Created on 09 March 2016, 21:08
 */

#ifndef NETSTRUCTURES_H
#define NETSTRUCTURES_H

#include <cassert>
#include <cinttypes>
#include <limits>
#include <vector>

namespace PetriEngine {

struct Arc {
    uint32_t _place;
    uint32_t _weight;
    bool _skip = false;
    bool _inhib = false;

    Arc()
        : _place(std::numeric_limits<uint32_t>::max()),
          _weight(std::numeric_limits<uint32_t>::max()), _skip(false), _inhib(false){};

    bool operator<(const Arc &other) const { return _place < other._place; }

    bool operator==(const Arc &other) const {
        return _place == other._place && _weight == other._weight && _inhib == other._inhib;
    }
};

struct Transition {
    std::vector<Arc> _pre;
    std::vector<Arc> _post;
    bool _skip = false;
    bool _inhib = false;

    void add_pre_arc(const Arc &arc) {
        auto lb = std::lower_bound(_pre.begin(), _pre.end(), arc);
        if (lb != _pre.end() && lb->_place == arc._place)
            lb->_weight += arc._weight;
        else
            lb = _pre.insert(lb, arc);
        assert(lb->_weight > 0);
    }

    void add_post_arc(const Arc &arc) {
        auto lb = std::lower_bound(_post.begin(), _post.end(), arc);
        if (lb != _post.end() && lb->_place == arc._place)
            lb->_weight += arc._weight;
        else
            lb = _post.insert(lb, arc);
        assert(lb->_weight > 0);
    }
};

struct Place {
    std::vector<uint32_t> _consumers; // things consuming
    std::vector<uint32_t> _producers; // things producing
    bool _skip = false;
    bool _inhib = false;

    // should be replaced using concepts in c++20
    void add_consumer(uint32_t id) {
        auto lb = std::lower_bound(_consumers.begin(), _consumers.end(), id);
        if (lb == _consumers.end() || *lb != id)
            _consumers.insert(lb, id);
    }

    void add_producer(uint32_t id) {
        auto lb = std::lower_bound(_producers.begin(), _producers.end(), id);
        if (lb == _producers.end() || *lb != id)
            _producers.insert(lb, id);
    }
};
} // namespace PetriEngine
#endif /* NETSTRUCTURES_H */
