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
        uint32_t pre = 0, post = 0;
        bool cycle = false;
    };
    struct trans_t {
        uint32_t index = 0;
        int8_t direction = 0;
        bool safe = true;
        trans_t() = default;
        trans_t(uint32_t id, int8_t dir) : index(id), direction(dir) {};
        bool operator<(const trans_t& t) const
        {
            return index < t.index;
        }
    };
    ReducingSuccessorGenerator(const PetriNet& net, bool is_game = false, bool is_safety = false);
    ReducingSuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries, bool is_game = false, bool is_safety = false);
    virtual ~ReducingSuccessorGenerator() = default;
    void prepare(const MarkVal* state, PetriNet::player_t player = PetriNet::ANY);
    bool next(MarkVal* write);
    void presetOf(uint32_t place, bool make_closure = false);
    void postsetOf(uint32_t place, bool make_closure = false);
    void postPresetOf(uint32_t t, bool make_closure = false);
    void inhibitorPostsetOf(uint32_t place);
    bool seenPre(uint32_t place) const;
    bool seenPost(uint32_t place) const;
    bool seenInhib(uint32_t place) const;
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
    std::unique_ptr<place_t[]> _places;
    std::unique_ptr<trans_t[]> _transitions;
    std::unique_ptr<uint32_t[]> _dependency;
    light_deque<uint32_t> _unprocessed, _ordering, _remaining;
    uint32_t _current;
    bool _netContainsInhibitorArcs;
    std::vector<std::vector<uint32_t>> _inhibpost;
    std::vector<PQL::Condition* > _queries;
    std::vector<uint32_t> _safety_orphans;
    bool _skip;
    bool _added_unsafe;
    bool _op_cand;
    PetriNet::player_t _players_enabled;
    std::vector<uint32_t> _env_trans;
    std::vector<uint32_t> _ctrl_trans;
    void constructEnabled();
    void constructPrePost();
    void constructDependency();
    void checkForInhibitor();
    void fireCurrent(MarkVal* write);
    
    struct tarjan_t {
        uint32_t index = 0;
        uint32_t lowlink = 0;
        bool on_stack = false;
    };
    void computeSafe();
    void computeStaticCycles();
    void computeSafetyOrphan();
    void computeSCC(uint32_t v, uint32_t& index, tarjan_t* data, std::stack<uint32_t>& stack);
    void preserveCycles();
    
    // for transitions
    static constexpr uint8_t ENABLED = 1;
    static constexpr uint8_t STUBBORN = 2;
    static constexpr uint8_t ADDED_POST = 4;
    
    // for places seen
    static constexpr uint8_t PRESET = 1;
    static constexpr uint8_t POSTSET = 2;
    static constexpr uint8_t INHIB = 4;
    static constexpr uint8_t CYCLE = 8;
    static constexpr uint8_t FRESH = 16;
    static constexpr uint8_t MARKED = 32;
    static constexpr uint8_t UNSAFE = 64;
};
}

#endif /* PETRIENGINE_REDUCINGSUCCESSORGENERATOR_H_ */
