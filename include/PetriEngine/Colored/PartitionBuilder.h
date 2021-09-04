#include "ColoredNetStructures.h"
#include "EquivalenceVec.h"
#include "IntervalGenerator.h"
 
namespace PetriEngine {
    namespace Colored {
        class PartitionBuilder {

            
            public:
                PartitionBuilder(const std::vector<Transition> &transitions, 
                                const std::vector<Place> &places, 
                                const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePostTransitionMap, 
                                const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePreTransitionMap);

                PartitionBuilder(const std::vector<Transition> &transitions, 
                                const std::vector<Place> &places, 
                                const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePostTransitionMap, 
                                const std::unordered_map<uint32_t,std::vector<uint32_t>> &placePreTransitionMap,
                                const std::vector<Colored::ColorFixpoint> *placeColorFixpoints);
                
                ~PartitionBuilder() {}

                //void initPartition();
                bool partition_net(int32_t timeout);
                void print_partion() const;
                void assign_color_map(std::unordered_map<uint32_t, EquivalenceVec> &partition) const;

                std::unordered_map<uint32_t, EquivalenceVec> get_partition() const{
                    return _partition;
                }

            private:
                const std::vector<Transition> &_transitions;
                const std::vector<Place> &_places;
                const std::unordered_map<uint32_t,std::vector<uint32_t>> &_placePostTransitionMap;
                const std::unordered_map<uint32_t,std::vector<uint32_t>> &_placePreTransitionMap;
                std::unordered_map<uint32_t,bool> _inQueue;
                std::unordered_map<uint32_t, EquivalenceVec> _partition;
                const PetriEngine::Colored::IntervalGenerator _interval_generator = IntervalGenerator();
                std::vector<uint32_t> _placeQueue;

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
