#ifndef PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_
#define PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_

#include "SuccessorGenerator.h"
#include "PQL/PQL.h"
#include "Utils/Structures/light_deque.h"

#include <memory>
#include <stack>
namespace PetriEngine {

class ReducingSuccessorGenerator : public SuccessorGenerator {

public:
    struct place_t {
        uint32_t pre, post;
    };
    struct trans_t {
        uint32_t index;
        int8_t direction;
        trans_t() = default;
        trans_t(uint32_t id, int8_t dir) : index(id), direction(dir) {};
        bool operator<(const trans_t& t) const
        {
            return index < t.index;
        }
    };
    ReducingSuccessorGenerator(const PetriNet& net, bool is_game = false);
    ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries, bool is_game = false);
    virtual ~ReducingSuccessorGenerator() = default;
    void prepare(const MarkVal* state, PetriNet::player_t player = PetriNet::ANY);
    bool next(MarkVal* write);
    void presetOf(uint32_t place, bool make_closure = false);
    void postsetOf(uint32_t place, bool make_closure = false);
    void postPresetOf(uint32_t t, bool make_closure = false);
    void inhibitorPostsetOf(uint32_t place);
    bool seenPre(uint32_t place) const;
    bool seenPost(uint32_t place) const;
    uint32_t leastDependentEnabled();
    uint32_t fired()
    {
       return _current;
    }
    void setQuery(PQL::Condition* ptr) { _queries.clear(); _queries = {ptr};}
    void reset();

private:
    inline void addToStub(uint32_t t);
    void closure();
    std::unique_ptr<uint8_t[]> _stub_enable;
    std::unique_ptr<uint8_t[]> _places_seen;
    std::unique_ptr<bool[]>    _cycle_places;
    std::unique_ptr<place_t[]> _places;
    std::unique_ptr<trans_t[]> _transitions;
    light_deque<uint32_t> _unprocessed, _ordering, _remaining;
    std::unique_ptr<uint32_t[]> _dependency;
    uint32_t _current;
    bool _netContainsInhibitorArcs;
    std::vector<std::vector<uint32_t>> _inhibpost;
    std::vector<PQL::Condition* > _queries;
    void constructEnabled();
    void constructPrePost();
    void constructDependency();
    void checkForInhibitor();
    
    struct tarjan_t {
        uint32_t index = 0;
        uint32_t lowlink = 0;
        bool on_stack = false;
    };
    void computeCycles();    
    void computeSCC(uint32_t v, uint32_t& index, tarjan_t* data, std::stack<uint32_t>& stack);
    
    static constexpr uint8_t ENABLED = 1;
    static constexpr uint8_t STUBBORN = 2;
};
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
