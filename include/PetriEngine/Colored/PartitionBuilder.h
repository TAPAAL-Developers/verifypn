
#ifndef PARTITION_BUILDER
#define PARTITION_BUILDER

#include "ColoredNetStructures.h"
#include "EquivalenceVec.h"
#include "IntervalGenerator.h"

#include <stack>

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    namespace Colored {
        class PartitionBuilder {

            public:
                PartitionBuilder(ColoredPetriNetBuilder& builder);

                ~PartitionBuilder() {}

                //void initPartition();
                bool partition(int32_t timeout);
                void print() const;

                const std::vector<EquivalenceVec>& get_partition() const {
                    return _partition;
                }

                double time() const { return _total_time; }

            private:
                const ColoredPetriNetBuilder& _builder;
                std::vector<bool> _inQueue;
                std::vector<EquivalenceVec> _partition;
                const PetriEngine::Colored::IntervalGenerator _interval_generator = IntervalGenerator();
                std::stack<uint32_t> _placeQueue;
                double _total_time = 0;


                void assign_color_map();
                bool split_partition(EquivalenceVec equivalenceVec, uint32_t placeId);
                void handle_transition(uint32_t transitionId, uint32_t postPlaceId);
                void handle_transition(const Transition &transitionId, const uint32_t postPlaceId, const Arc *postArc);

                void handle_leaf_transitions();

                void add_to_queue(uint32_t placeId);

                bool check_tuple_diagonal(uint32_t placeId);
                bool check_diagonal(uint32_t placeId);

                void handle_in_arcs(const Transition &transition, std::set<const Colored::Variable*> &diagonalVars, const PositionVariableMap &varPositionMap, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps, uint32_t postPlaceId);

                void apply_new_intervals(const Arc &inArc, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps);

                void check_var_on_arc(const VariableModifierMap &varModifierMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId, bool inputArc);

                void check_var_on_input_arcs(const std::unordered_map<uint32_t,PositionVariableMap> &placeVariableMap, const PositionVariableMap &preVarPositionMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId);

                void mark_shared_vars(const PositionVariableMap &preVarPositionMap, const PositionVariableMap &varPositionMap, uint32_t postPlaceId, uint32_t prePlaceId);
                void check_var_in_guard(const PositionVariableMap &preVarPositionMap, const std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId);

                std::vector<VariableIntervalMap> prepare_variables(
                            const VariableModifierMap &varModifierMap,
                            const EquivalenceClass& eqClass , const Arc *postArc, uint32_t placeId);

                bool find_overlap(const EquivalenceVec &equivalenceVec1,const EquivalenceVec &equivalenceVec2, uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection);

                uint32_t _eq_id_counter = 0;

        };
    }
}
#endif