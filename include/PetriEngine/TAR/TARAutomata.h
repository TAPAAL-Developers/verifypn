/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   TARAutomata.h
 * Author: petko
 *
 * Created on January 2, 2018, 10:06 AM
 */

#ifndef TARAUTOMATA_H
#define TARAUTOMATA_H
#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <vector>

#include "PetriEngine/PetriNet.h"
#include "range.h"

namespace PetriEngine {
namespace Reachability {
class AutomataState;

struct AutomataEdge {
    size_t _edge;
    std::vector<size_t> _to;

    bool operator==(const AutomataEdge &other) const { return _edge == other._edge; }

    bool operator!=(const AutomataEdge &other) const { return !(*this == other); }

    bool operator<(const AutomataEdge &other) const { return _edge < other._edge; };

    AutomataEdge(size_t e) : _edge(e){};

    AutomataEdge(const AutomataEdge &other) = default;
    AutomataEdge(AutomataEdge &&) = default;
    AutomataEdge &operator=(const AutomataEdge &other) = default;
    AutomataEdge &operator=(AutomataEdge &&other) = default;

    bool has_to(size_t i) {
        if (_to.size() > 0 && _to[0] == 0)
            return true;
        auto lb = std::lower_bound(_to.begin(), _to.end(), i);
        return lb != _to.end() && *lb == i;
    }

    bool add(size_t i) {
        if (_to.size() > 0 && _to[0] == 0)
            return false;
        if (i == 0) {
            _to.clear();
            _to.push_back(0);
            return true;
        }

        auto lb = std::lower_bound(_to.begin(), _to.end(), i);
        if (lb != _to.end() && *lb == i) {
            return false;
        } else {
            _to.insert(lb, i);
            return true;
        }
    }

    bool remove(size_t i) {
        auto lb = std::lower_bound(_to.begin(), _to.end(), i);
        if (lb == _to.end() || *lb != i) {
            return false;
        } else {
            _to.erase(lb);
            return true;
        }
    }

    std::ostream &operator<<(std::ostream &os) const {
        os << _edge << " ==> ";
        for (size_t i : _to)
            os << i << ", ";
        return os;
    }
};
class TraceSet;
struct AutomataState {
  private:
    std::vector<AutomataEdge> _edges;
    bool _accept = false;
    std::vector<size_t> _simulates;
    std::vector<size_t> _simulators;
    friend class TraceSet;

  public:
    prvector_t _interpolant;
    AutomataState(prvector_t interpol) : _interpolant(interpol){};
    inline bool is_accepting() const { return _accept; }

    inline void set_accepting(bool val = true) { _accept = val; }

    inline bool has_edge(const size_t &e, size_t to) {
        AutomataEdge edge(e);
        auto lb = std::lower_bound(_edges.begin(), _edges.end(), edge);
        if (lb == _edges.end() || *lb != edge) {
            return false;
        }
        return lb->has_to(to);
    }

    inline bool has_any_edge(size_t prod, const size_t &e) {
        AutomataEdge edge(e);
        auto lb = std::lower_bound(_edges.begin(), _edges.end(), edge);
        if (lb == _edges.end() || *lb != edge) {
            return false;
        }
        return lb->_to.size() > 0;
    }

    inline bool add_edge(const size_t &e, size_t to) {
        AutomataEdge edge(e);
        auto lb = std::lower_bound(_edges.begin(), _edges.end(), edge);
#ifndef NDEBUG
        bool isnew = false;
#endif
        if (lb == _edges.end() || *lb != edge) {
            assert(lb == _edges.end() || *lb != edge);
            lb = _edges.insert(lb, edge);
#ifndef NDEBUG
            isnew = true;
#endif
        }
        assert(*lb == edge);
        assert(is_sorted(_edges.begin(), _edges.end()));
        bool res = lb->add(to);
        assert(!isnew || res);
        assert(lb->_to.size() >= 0);
        return res;
    }

    inline bool remove_edge(size_t e) {
        AutomataEdge edge(e);
        auto lb = std::lower_bound(_edges.begin(), _edges.end(), edge);
        if (lb == _edges.end() || *lb != edge)
            return false;
        _edges.erase(lb);
        return true;
    }

    inline bool remove_edge(size_t e, size_t to) {
        AutomataEdge edge(e);
        auto lb = std::lower_bound(_edges.begin(), _edges.end(), edge);
        if (lb == _edges.end() || *lb != edge) {
            assert(lb == _edges.end() || *lb != edge);
            assert(is_sorted(_edges.begin(), _edges.end()));
            return false;
        }
        assert(*lb == edge);
        assert(is_sorted(_edges.begin(), _edges.end()));
        bool removed = lb->remove(to);
        if (removed && lb->_to.size() == 0) {
            _edges.erase(lb);
        } else {
            assert(lb->_to.size() >= 0);
        }
        return removed;
    }

    inline auto first_edge(size_t &e) const {
        AutomataEdge edge(e);
        auto lb = std::lower_bound(_edges.begin(), _edges.end(), edge);
        return lb;
    }

    inline auto last_edge() const { return _edges.end(); }

    inline auto &get_edges() const { return _edges; }

    std::ostream &print(std::ostream &os) const {
        os << "\t PREDICATE\n";
        _interpolant.print(os);
        os << "\n";
        os << "\tSIMS [ ";
        for (auto s : _simulates)
            os << s << ", ";
        os << "]\n\tSIMED [";
        for (auto s : _simulators)
            os << s << ", ";
        os << "]\n";
        os << "\t EDGES\n";
        for (auto &e : _edges) {
            os << "\t\t<" << (e._edge ? std::to_string(e._edge - 1) : "Q") << "> --> [";
            for (auto t : e._to)
                os << t << ", ";
            os << "]\n";
        }
        return os;
    }
};

class state_t {

  private:
    size_t _offset = 0;
    size_t _size = std::numeric_limits<size_t>::max();
    size_t _edgecnt = 0;
    std::set<size_t> _interpolant;

  public:
    bool operator==(const state_t &other) {
        //                if((get_edge_cnt() == 0) != (other.get_edge_cnt() == 0)) return false;
        if (_interpolant.size() == other._interpolant.size() &&
            std::equal(_interpolant.begin(), _interpolant.end(), other._interpolant.begin(),
                       other._interpolant.end())) {
            return true;
        }
        return false;
    }

    bool operator!=(const state_t &other) { return !(*this == other); }

    bool operator<=(const state_t &other) {
        //                if((get_edge_cnt() == 0) != (other.get_edge_cnt() == 0)) return false;
        if (_interpolant.size() <= other._interpolant.size() &&
            std::includes(other._interpolant.begin(), other._interpolant.end(),
                          _interpolant.begin(), _interpolant.end())) {
            return true;
        }
        return false;
    }

    size_t get_edge_cnt() {
        if (_edgecnt == 0)
            return 0;
        else
            return 1 + (((_edgecnt - 1) + _offset) % _size);
    }

    void set_edge(size_t edge) {
        _edgecnt = edge;
        _offset = 0;
    }

    bool next_edge(const PetriNet &net) {
        ++_edgecnt;
        return done(net);
    }

    bool done(const PetriNet &net) const { return _edgecnt > net.number_of_transitions(); }

    bool reset_edges(const PetriNet &net) {
        _size = net.number_of_transitions();
        _edgecnt = 0;
        _offset = std::rand(); // % (net.number_of_transitions());
        return _edgecnt > net.number_of_transitions();
    }

    inline void add_interpolant(size_t ninter) { _interpolant.insert(ninter); }

    inline std::set<size_t> &get_interpolants() { return _interpolant; }

    inline void set_interpolants(const std::set<size_t> &interpolants) {
        _interpolant = interpolants;
    }
};

typedef std::vector<state_t> trace_t;
} // namespace Reachability
} // namespace PetriEngine

#endif /* TARAUTOMATA_H */
