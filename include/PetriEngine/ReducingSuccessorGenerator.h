#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "PQL/PQL.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include "Structures/LightDequeue.h"
#include "Structures/State.h"
#include "SuccessorGenerator.h"
#include <memory>

namespace LTL {
class GuardInfo;
}
namespace PetriEngine {

class ReducingSuccessorGenerator : public SuccessorGenerator {

  public:
    ReducingSuccessorGenerator(const PetriNet &net, std::shared_ptr<StubbornSet> stubbornSet);

    ReducingSuccessorGenerator(const PetriNet &net,
                               std::vector<std::shared_ptr<PQL::Condition>> &queries,
                               std::shared_ptr<StubbornSet> stubbornSet);

    void reset();

    void set_query(PQL::Condition *ptr) { _stubSet->set_query(ptr); }

    auto prepare(const Structures::State &state) -> bool override;

    auto next(Structures::State &write) -> bool override;

    [[nodiscard]] auto fired() const -> uint32_t override { return _current; }

  private:
    std::shared_ptr<StubbornSet> _stubSet;
    uint32_t _current;

    std::vector<PQL::Condition *> _queries;
};
} // namespace PetriEngine

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
