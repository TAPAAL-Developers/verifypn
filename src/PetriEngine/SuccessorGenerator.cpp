/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SuccessorGenerator.cpp
 * Author: Peter G. Jensen
 *
 * Created on 30 March 2016, 19:50
 */

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/Structures/State.h"
#include "errorcodes.h"

#include <cassert>
namespace PetriEngine {

SuccessorGenerator::SuccessorGenerator(const PetriNet &net) : _net(net), _parent(nullptr) {
    reset();
}
SuccessorGenerator::SuccessorGenerator(const PetriNet &net,
                                       const std::vector<std::shared_ptr<PQL::Condition>> &queries)
    : SuccessorGenerator(net) {}

SuccessorGenerator::SuccessorGenerator(const PetriNet &net,
                                       const std::shared_ptr<PQL::Condition> &query)
    : SuccessorGenerator(net) {}

SuccessorGenerator::~SuccessorGenerator() = default;

auto SuccessorGenerator::prepare(const Structures::State &state) -> bool {
    _parent = &state;
    reset();
    return true;
}

void SuccessorGenerator::reset() {
    _suc_pcounter = 0;
    _suc_tcounter = std::numeric_limits<uint32_t>::max();
}

void SuccessorGenerator::consume_preset(Structures::State &write, uint32_t t) {

    const trans_ptr_t &ptr = _net._transitions[t];
    uint32_t finv = ptr._inputs;
    uint32_t linv = ptr._outputs;
    for (; finv < linv; ++finv) {
        if (!_net._invariants[finv]._inhibitor) {
            assert(write.marking()[_net._invariants[finv]._place] >=
                   _net._invariants[finv]._tokens);
            write.marking()[_net._invariants[finv]._place] -= _net._invariants[finv]._tokens;
        }
    }
}

auto SuccessorGenerator::check_preset(uint32_t t) -> bool {
    const trans_ptr_t &ptr = _net._transitions[t];
    uint32_t finv = ptr._inputs;
    uint32_t linv = ptr._outputs;

    for (; finv < linv; ++finv) {
        const invariant_t &inv = _net._invariants[finv];
        if ((*_parent).marking()[inv._place] < inv._tokens) {
            if (!inv._inhibitor) {
                return false;
            }
        } else {
            if (inv._inhibitor) {
                return false;
            }
        }
    }
    return true;
}

void SuccessorGenerator::produce_postset(Structures::State &write, uint32_t t) {
    const trans_ptr_t &ptr = _net._transitions[t];
    uint32_t finv = ptr._outputs;
    uint32_t linv = _net._transitions[t + 1]._inputs;

    for (; finv < linv; ++finv) {
        size_t n = write.marking()[_net._invariants[finv]._place];
        n += _net._invariants[finv]._tokens;
        if (n >= std::numeric_limits<uint32_t>::max()) {
            throw base_error_t("Exceeded 2**32 limit of tokens in a single place (", n, ")");
        }
        write.marking()[_net._invariants[finv]._place] = n;
    }
}

auto SuccessorGenerator::next(Structures::State &write) -> bool {
    for (; _suc_pcounter < _net._nplaces; ++_suc_pcounter) {
        // orphans are currently under "place 0" as a special case
        if (_suc_pcounter == 0 || (*_parent).marking()[_suc_pcounter] > 0) {
            if (_suc_tcounter == std::numeric_limits<uint32_t>::max()) {
                _suc_tcounter = _net._placeToPtrs[_suc_pcounter];
            }
            uint32_t last = _net._placeToPtrs[_suc_pcounter + 1];
            for (; _suc_tcounter != last; ++_suc_tcounter) {

                if (!check_preset(_suc_tcounter))
                    continue;
                memcpy(write.marking(), (*_parent).marking(), _net._nplaces * sizeof(MarkVal));
                consume_preset(write, _suc_tcounter);
                produce_postset(write, _suc_tcounter);

                ++_suc_tcounter;
                return true;
            }
            _suc_tcounter = std::numeric_limits<uint32_t>::max();
        }
        _suc_tcounter = std::numeric_limits<uint32_t>::max();
    }
    return false;
}

auto SuccessorGenerator::next(Structures::State &write, uint32_t &tindex) -> bool {
    _parent = &write;
    _suc_pcounter = 0;
    for (; _suc_pcounter < _net._nplaces; ++_suc_pcounter) {
        // orphans are currently under "place 0" as a special case
        if (_suc_pcounter == 0 || (*_parent).marking()[_suc_pcounter] > 0) {
            if (tindex == std::numeric_limits<uint32_t>::max()) {
                tindex = _net._placeToPtrs[_suc_pcounter];
            }
            uint32_t last = _net._placeToPtrs[_suc_pcounter + 1];
            for (; tindex != last; ++tindex) {

                if (!check_preset(tindex))
                    continue;
                fire(write, tindex);

                ++tindex;
                return true;
            }
            tindex = std::numeric_limits<uint32_t>::max();
        }
        tindex = std::numeric_limits<uint32_t>::max();
    }
    return false;
}

void SuccessorGenerator::fire(Structures::State &write, uint32_t tid) {
    assert(check_preset(tid));
    memcpy(write.marking(), (*_parent).marking(), _net._nplaces * sizeof(MarkVal));
    consume_preset(write, tid);
    produce_postset(write, tid);
}

SuccessorGenerator::SuccessorGenerator(const PetriNet &net, const std::shared_ptr<StubbornSet> &)
    : SuccessorGenerator(net) {}

} // namespace PetriEngine
