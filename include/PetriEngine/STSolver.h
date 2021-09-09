#ifndef STSOLVER_H
#define STSOLVER_H
#include "Reachability/ReachabilityResult.h"
#include "Structures/State.h"
#include "TAR/AntiChain.h"

#include <chrono>
#include <memory>

namespace PetriEngine {
class STSolver {
    struct place_t {
        uint32_t _pre, _post;
    };

  public:
    STSolver(const Reachability::ResultPrinter &printer, const PetriNet &net,
             const PQL::Condition &query, uint32_t depth);
    virtual ~STSolver();
    auto solve(uint32_t timeout) -> bool;
    auto print_result() -> Reachability::ResultPrinter::result_e;

  private:
    auto compute_trap(std::vector<size_t> &siphon, const std::set<size_t> &pre,
                      const std::set<size_t> &post, size_t marked_count) -> size_t;
    auto siphon_trap(std::vector<size_t> siphon, const std::vector<bool> &has_st,
                     const std::set<size_t> &pre, const std::set<size_t> &post) -> bool;
    [[nodiscard]] auto duration() const -> uint32_t;
    [[nodiscard]] auto timeout() const -> bool;
    void construct_pre_post();
    void extend(size_t place, std::set<size_t> &pre, std::set<size_t> &post);
    bool _siphonPropperty = false;
    const Reachability::ResultPrinter &_printer;
    const PQL::Condition &_query;
    std::unique_ptr<place_t[]> _places;
    std::unique_ptr<uint32_t[]> _transitions;
    std::vector<size_t> _diff;
    const PetriNet &_net;
    const MarkVal *_m0;
    uint32_t _siphonDepth;
    uint32_t _timelimit;
    uint32_t _analysisTime;
    std::chrono::high_resolution_clock::time_point _start;
    AntiChain<size_t, size_t> _antichain;
};
} // namespace PetriEngine
#endif /* STSOLVER_H */
