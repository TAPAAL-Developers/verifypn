#ifndef STSOLVER_H
#define STSOLVER_H
#include "Structures/State.h"
#include "Reachability/ReachabilityResult.h"
#include "TAR/AntiChain.h"

#include <memory>
#include <chrono>

namespace PetriEngine {
    class STSolver {
    struct place_t {
        uint32_t _pre, _post;
    };

    public:
        STSolver(const Reachability::ResultPrinter& printer, const PetriNet& net, PQL::Condition * query, uint32_t depth);
        virtual ~STSolver();
        bool solve(uint32_t timeout);
        Reachability::ResultPrinter::Result print_result();

    private:
        size_t _compute_trap(std::vector<size_t>& siphon, const std::set<size_t>& pre, const std::set<size_t>& post, size_t marked_count);
        bool siphon_trap(std::vector<size_t> siphon, const std::vector<bool>& has_st, const std::set<size_t>& pre, const std::set<size_t>& post);
        uint32_t duration() const;
        bool timeout() const;
        void construct_pre_post();
        void extend(size_t place, std::set<size_t>& pre, std::set<size_t>& post);
        bool _siphonPropperty = false;
        const Reachability::ResultPrinter& _printer;
        PQL::Condition * _query;
        std::unique_ptr<place_t[]> _places;
        std::unique_ptr<uint32_t[]> _transitions;
        std::vector<size_t> _diff;
        const PetriNet& _net;
        const MarkVal* _m0;
        uint32_t _siphonDepth;
        uint32_t _timelimit;
        uint32_t _analysisTime;
        std::chrono::high_resolution_clock::time_point _start;
        AntiChain<size_t, size_t> _antichain;
    };
}
#endif /* STSOLVER_H */

