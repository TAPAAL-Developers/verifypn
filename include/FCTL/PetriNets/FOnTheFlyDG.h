#ifndef FEATURED_ONTHEFLYDG_H
#define FEATURED_ONTHEFLYDG_H

#include "logging.h"

#include <functional>
#include <stack>
#include <ptrie/ptrie_map.h>

#include "FCTL/DependencyGraph/FBasicDependencyGraph.h"
#include "FCTL/DependencyGraph/FConfiguration.h"
#include "FCTL/DependencyGraph/FeaturedEdge.h"
#include "PetriConfig.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/Structures/AlignedEncoder.h"
#include "PetriEngine/Structures/linked_bucket.h"
#include "FCTL/FeaturedPN/FeaturedSuccessorGenerator.h"



namespace Featured {
    namespace PetriNets {
        class FOnTheFlyDG : public DependencyGraph::BasicDependencyGraph {
        public:
            using Condition = PetriEngine::PQL::Condition;
            using Condition_ptr = PetriEngine::PQL::Condition_ptr;
            using Marking = PetriEngine::Structures::State;

            FOnTheFlyDG(PetriEngine::PetriNet* t_net, bool partial_order);

            virtual ~FOnTheFlyDG();

            //Dependency graph interface
            virtual std::vector<DependencyGraph::Edge*> successors(DependencyGraph::Configuration* c) override;

            virtual DependencyGraph::Configuration* initialConfiguration() override;

            virtual void cleanUp() override;

            void setQuery(Condition* query);

            virtual void release(DependencyGraph::Edge* e) override;

            size_t owner(Marking& marking, Condition* cond);

            size_t owner(Marking& marking, const Condition_ptr& cond) {
                return owner(marking, cond.get());
            }


            //stats
            size_t configurationCount() const;

            size_t markingCount() const;

            size_t maxTokens() const;

            Condition::Result initialEval();

            virtual void print(DependencyGraph::Configuration* c, std::ostream& out = std::cerr) override;

        protected:

            //initialized from constructor
            AlignedEncoder encoder;
            PetriEngine::PetriNet* net = nullptr;
            PetriConfig* initial_config;
            Marking working_marking;
            Marking query_marking;
            uint32_t n_transitions = 0;
            uint32_t n_places = 0;
            size_t _markingCount = 0;
            size_t _maxTokens = 0;
            size_t _configurationCount = 0;
            //used after query is set
            Condition* query = nullptr;

            Condition::Result fastEval(Condition* query, Marking* unfolded);

            Condition::Result fastEval(const Condition_ptr& query, Marking* unfolded) {
                return fastEval(query.get(), unfolded);
            }

            void nextStates(Marking& t_marking, Condition*,
                            std::function<void()> pre,
                            std::function<bool(Marking&, bdd)> foreach,
                            std::function<void()> post);

            void dowork(FeaturedSuccessorGenerator& gen, bool& first,
                        std::function<void()>& pre,
                        std::function<bool(Marking&, bdd)>& foreach) {
                gen.prepare(&query_marking);

                while (gen.next(working_marking)) {
                    if (first) pre();
                    first = false;
                    bdd feat = gen.feature();
                    if (!foreach(working_marking, feat)) {
                        gen.reset();
                        break;
                    }
                }
            }

            PetriConfig* createConfiguration(size_t marking, size_t own, Condition* query);

            PetriConfig* createConfiguration(size_t marking, size_t own, const Condition_ptr& query) {
                return createConfiguration(marking, own, query.get());
            }

            size_t createMarking(Marking& marking);

            void markingStats(const uint32_t* marking, size_t& sum, bool& allsame, uint32_t& val, uint32_t& active,
                              uint32_t& last);

            DependencyGraph::Edge* newEdge(DependencyGraph::Configuration& t_source, uint32_t weight);

            std::stack<DependencyGraph::Edge*> recycle;
            ptrie::map<ptrie::uchar, std::vector<PetriConfig*> > trie;
            linked_bucket_t<DependencyGraph::Edge, 1024 * 10>* edge_alloc = nullptr;

            // Problem  with linked bucket and complex constructor
            linked_bucket_t<char[sizeof(PetriConfig)], 1024 * 1024>* conf_alloc = nullptr;

            Featured::FeaturedSuccessorGenerator gen_;
            bool _partial_order = false;

        };
    }
}
#endif // ONTHEFLYDG_H
