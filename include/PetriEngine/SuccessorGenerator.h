/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SuccessorGenerator.h
 * Author: Peter G. Jensen
 *
 * Created on 30 March 2016, 19:50
 */

#ifndef SUCCESSORGENERATOR_H
#define SUCCESSORGENERATOR_H

#include "PetriNet.h"
#include "Structures/State.h"
#include "Stubborn/StubbornSet.h"
#include <memory>

namespace PetriEngine {

class SuccessorGenerator {
  public:
    SuccessorGenerator(const PetriNet &net);
    SuccessorGenerator(const PetriNet &net, const std::shared_ptr<StubbornSet> &);
    SuccessorGenerator(const PetriNet &net,
                       const std::vector<std::shared_ptr<PQL::Condition>> &queries);
    SuccessorGenerator(const PetriNet &net, const std::shared_ptr<PQL::Condition> &query);
    virtual ~SuccessorGenerator();
    virtual auto prepare(const Structures::State &state) -> bool;
    virtual auto next(Structures::State &write) -> bool;
    [[nodiscard]] virtual auto fired() const -> uint32_t {
        return _suc_tcounter == std::numeric_limits<uint32_t>::max()
                   ? std::numeric_limits<uint32_t>::max()
                   : _suc_tcounter - 1;
    }

    [[nodiscard]] virtual auto get_parent() const -> const MarkVal * { return _parent->marking(); }

    void reset();

    /**
     * Checks if the conditions are met for fireing t, if write != nullptr,
     * then also consumes tokens from write while checking
     * @param t, transition to fire
     * @param write, marking to consume from (possibly nullptr)
     * @return true if t is fireable, false otherwise
     */
    auto check_preset(uint32_t t) -> bool;

    /**
     * Consumes tokens in preset of t without from marking write checking
     * @param write, a marking to consume from
     * @param t, a transition to fire
     */
    void consume_preset(Structures::State &write, uint32_t t);

    /**
     * Produces tokens in write, given by t
     * @param write, a marking to produce to
     * @param t, a transition to fire
     */
    void produce_postset(Structures::State &write, uint32_t t);

  protected:
    const PetriNet &_net;

    auto next(Structures::State &write, uint32_t &tindex) -> bool;

    void fire(Structures::State &write, uint32_t tid);

    const Structures::State *_parent;

    uint32_t _suc_pcounter;
    uint32_t _suc_tcounter;

  private:
    friend class ReducingSuccessorGenerator;
};
} // namespace PetriEngine

#endif /* SUCCESSORGENERATOR_H */
