/*
 * File:   Reducer.h
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#ifndef REDUCER_H
#define REDUCER_H

#include "../PetriParse/PNMLParser.h"
#include "NetStructures.h"
#include "PQL/Contexts.h"
#include "PetriNet.h"

#include <optional>
#include <vector>

namespace PetriEngine {

using ArcIter = std::vector<arc_t>::iterator;

class PetriNetBuilder;

class QueryPlaceAnalysisContext : public PQL::AnalysisContext {
    std::vector<uint32_t> _placeInQuery;
    bool _deadlock;

  public:
    QueryPlaceAnalysisContext(const std::unordered_map<std::string, uint32_t> &pnames,
                              const std::unordered_map<std::string, uint32_t> &tnames,
                              const PetriNet *net)
        : PQL::AnalysisContext(pnames, tnames, net) {
        _placeInQuery.resize(_placeNames.size(), 0);
        _deadlock = false;
    }

    virtual ~QueryPlaceAnalysisContext() {}

    uint32_t *get_query_placeCount() { return _placeInQuery.data(); }

    bool has_deadlock() { return _deadlock; }

    virtual void set_has_deadlock() override { _deadlock = true; };

    resolution_result_t resolve(const std::string &identifier, bool place) override {
        if (!place)
            return PQL::AnalysisContext::resolve(identifier, false);
        resolution_result_t result;
        result._offset = -1;
        result._success = false;
        auto it = _placeNames.find(identifier);
        if (it != _placeNames.end()) {
            uint32_t i = it->second;
            result._offset = (int)i;
            result._success = true;
            _placeInQuery[i]++;
            return result;
        }

        return result;
    }
};

struct ExpandedArc {
    ExpandedArc(std::string place, size_t weight) : _place(place), _weight(weight) {}

    friend std::ostream &operator<<(std::ostream &os, ExpandedArc const &ea) {
        for (size_t i = 0; i < ea._weight; ++i) {
            os << "\t\t<token place=\"" << ea._place << "\" age=\"0\"/>\n";
        }
        return os;
    }

    std::string _place;
    size_t _weight;
};

class Reducer {
  public:
    Reducer(PetriNetBuilder *);
    ~Reducer();
    void print(QueryPlaceAnalysisContext &context); // prints the net, just for debugging
    void reduce(QueryPlaceAnalysisContext &context, int enablereduction, bool reconstructTrace,
                int timeout, bool remove_loops, bool remove_consumers, bool next_safe,
                std::vector<uint32_t> &reductions);

    size_t removed_transitions() const { return _removedTransitions; }

    size_t removed_places() const { return _removedPlaces; }

    void print_stats(std::ostream &out) {
        out << "Removed transitions: " << _removedTransitions << "\n"
            << "Removed places: " << _removedPlaces << "\n"
            << "Applications of rule A: " << _ruleA << "\n"
            << "Applications of rule B: " << _ruleB << "\n"
            << "Applications of rule C: " << _ruleC << "\n"
            << "Applications of rule D: " << _ruleD << "\n"
            << "Applications of rule E: " << _ruleE << "\n"
            << "Applications of rule F: " << _ruleF << "\n"
            << "Applications of rule G: " << _ruleG << "\n"
            << "Applications of rule H: " << _ruleH << "\n"
            << "Applications of rule I: " << _ruleI << "\n"
            << "Applications of rule J: " << _ruleJ << "\n"
            << "Applications of rule K: " << _ruleK << std::endl;
    }

    void post_fire(std::ostream &, const std::string &transition) const;
    void extra_consume(std::ostream &, const std::string &transition) const;
    void init_fire(std::ostream &) const;

  private:
    size_t _removedTransitions = 0;
    size_t _removedPlaces = 0;
    size_t _ruleA = 0, _ruleB = 0, _ruleC = 0, _ruleD = 0, _ruleE = 0, _ruleF = 0, _ruleG = 0,
           _ruleH = 0, _ruleI = 0, _ruleJ = 0, _ruleK = 0;
    PetriNetBuilder *_builder = nullptr;
    bool _reconstruct_trace = false;
    std::chrono::high_resolution_clock::time_point _timer;
    int _timeout = 0;

    // The reduction methods return true if they reduced something and reductions should continue
    // with other rules
    bool rule_a(uint32_t *placeInQuery);
    bool rule_b(uint32_t *placeInQuery, bool remove_deadlocks, bool remove_consumers);
    bool rule_c(uint32_t *placeInQuery);
    bool rule_d(uint32_t *placeInQuery);
    bool rule_e(uint32_t *placeInQuery);
    bool rule_i(uint32_t *placeInQuery, bool remove_loops, bool remove_consumers);
    bool rule_f(uint32_t *placeInQuery);
    bool rule_g(uint32_t *placeInQuery, bool remove_loops, bool remove_consumers);
    bool rule_h(uint32_t *placeInQuery);
    bool rule_j(uint32_t *placeInQuery);
    bool rule_k(uint32_t *placeInQuery, bool remove_consumers);

    std::optional<std::pair<std::vector<bool>, std::vector<bool>>>
    relevant(const uint32_t *placeInQuery, bool remove_consumers);
    bool remove_irrelevant(const uint32_t *placeInQuery, const std::vector<bool> &tseen,
                           const std::vector<bool> &pseen);

    std::string get_transition_name(uint32_t transition);
    std::string get_place_name(uint32_t place);

    PetriEngine::transition_t &get_transition(uint32_t transition);
    ArcIter get_out_arc(PetriEngine::transition_t &, uint32_t place);
    ArcIter get_in_arc(uint32_t place, PetriEngine::transition_t &);
    void erase_transition(std::vector<uint32_t> &, uint32_t);
    void skip_transition(uint32_t);
    void skip_place(uint32_t);
    std::string new_trans_name();

    bool consistent();
    bool has_timed_out() const {
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _timer);
        return (diff.count() >= _timeout);
    }

    std::vector<std::string> _initfire;
    std::unordered_map<std::string, std::vector<std::string>> _postfire;
    std::unordered_map<std::string, std::vector<ExpandedArc>> _extraconsume;
    std::vector<uint8_t> _tflags;
    std::vector<uint8_t> _pflags;
    size_t _tnameid = 0;
    std::vector<uint32_t> _skipped_trans;
};

} // namespace PetriEngine

#endif /* REDUCER_H */
