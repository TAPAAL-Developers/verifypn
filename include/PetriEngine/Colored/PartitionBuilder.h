#include "ColoredNetStructures.h"
#include "EquivalenceVec.h"
#include "IntervalGenerator.h"

namespace PetriEngine::Colored {
class PartitionBuilder {

  public:
    PartitionBuilder(
        const std::vector<transition_t> &transitions, const std::vector<place_t> &places,
        const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePostTransitionMap,
        const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePreTransitionMap);

    PartitionBuilder(
        const std::vector<transition_t> &transitions, const std::vector<place_t> &places,
        const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePostTransitionMap,
        const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePreTransitionMap,
        const std::vector<Colored::color_fixpoint_t> *placecolor_fixpoint_ts);

    ~PartitionBuilder() = default;

    // void initPartition();
    auto partition_net(int32_t timeout) -> bool;
    void print_partion() const;
    void assign_color_map(std::unordered_map<uint32_t, EquivalenceVec> &partition) const;

    auto get_partition() const -> std::unordered_map<uint32_t, EquivalenceVec> {
        return _partition;
    }

  private:
    const std::vector<transition_t> &_transitions;
    const std::vector<place_t> &_places;
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &_placePostTransitionMap;
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &_placePreTransitionMap;
    std::unordered_map<uint32_t, bool> _inQueue;
    std::unordered_map<uint32_t, EquivalenceVec> _partition;
    const PetriEngine::Colored::IntervalGenerator _interval_generator = IntervalGenerator();
    std::vector<uint32_t> _placeQueue;

    auto split_partition(EquivalenceVec equivalenceVec, uint32_t placeId) -> bool;

    void handle_transition(uint32_t transitionId, uint32_t postPlaceId);
    void handle_transition(const transition_t &transitionId, const uint32_t postPlaceId,
                           const arc_t *postArc);

    void handle_leaf_transitions();

    void add_to_queue(uint32_t placeId);

    auto check_tuple_diagonal(uint32_t placeId) -> bool;
    auto check_diagonal(uint32_t placeId) -> bool;

    void handle_in_arcs(const transition_t &transition,
                        std::set<const Colored::Variable *> &diagonalVars,
                        const PositionVariableMap &varPositionMap,
                        const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps,
                        uint32_t postPlaceId);

    void apply_new_intervals(const arc_t &inArc,
                             const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps);

    void check_var_on_arc(const VariableModifierMap &varModifierMap,
                          std::set<const Colored::Variable *> &diagonalVars, uint32_t placeId,
                          bool inputArc);

    void check_var_on_input_arcs(
        const std::unordered_map<uint32_t, PositionVariableMap> &placeVariableMap,
        const PositionVariableMap &preVarPositionMap,
        std::set<const Colored::Variable *> &diagonalVars, uint32_t placeId);

    void mark_shared_vars(const PositionVariableMap &preVarPositionMap,
                          const PositionVariableMap &varPositionMap, uint32_t postPlaceId,
                          uint32_t prePlaceId);
    void check_var_in_guard(const PositionVariableMap &preVarPositionMap,
                            const std::set<const Colored::Variable *> &diagonalVars,
                            uint32_t placeId);

    auto prepare_variables(const VariableModifierMap &varModifierMap,
                           const EquivalenceClass &eqClass, const arc_t *postArc, uint32_t placeId)
        -> std::vector<VariableIntervalMap>;

    auto find_overlap(const EquivalenceVec &equivalenceVec1, const EquivalenceVec &equivalenceVec2,
                      uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection)
        -> bool;

    uint32_t _eq_id_counter = 0;
};
} // namespace PetriEngine::Colored
