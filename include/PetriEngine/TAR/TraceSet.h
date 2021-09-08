/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   TraceSet.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 2, 2020, 6:03 PM
 */

#ifndef TRACESET_H
#define TRACESET_H

#include "PetriEngine/PetriNet.h"
#include "TARAutomata.h"
#include "range.h"

#include <cinttypes>
#include <map>
#include <vector>

namespace PetriEngine::Reachability {
void inline_union(std::vector<size_t> &into, const std::vector<size_t> &other);
class TraceSet {
  public:
    TraceSet(const PetriNet &net);
    void clear();
    auto add_trace(std::vector<std::pair<prvector_t, size_t>> &inter) -> bool;
    void copy_non_changed(const std::set<size_t> &from, const std::vector<int64_t> &modifiers,
                          std::set<size_t> &to) const;
    auto follow(const std::set<size_t> &from, std::set<size_t> &nextinter, size_t symbol) -> bool;
    [[nodiscard]] auto maximize(const std::set<size_t> &from) const -> std::set<size_t>;
    [[nodiscard]] auto minimize(const std::set<size_t> &from) const -> std::set<size_t>;
    [[nodiscard]] auto initial() const -> std::set<size_t> { return _initial; }
    auto print(std::ostream &out) const -> std::ostream &;
    void remove_edge(size_t edge);

  private:
    void init();
    auto state_for_predicate(prvector_t &predicate) -> std::pair<bool, size_t>;
    void compute_simulation(size_t index);
    std::map<prvector_t, size_t> _intmap;
    std::vector<AutomataState> _states;
    std::set<size_t> _initial;
    const PetriNet &_net;
};

} // namespace PetriEngine::Reachability

#endif /* TRACESET_H */
