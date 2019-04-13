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
#include <memory>

namespace PetriEngine {
class SuccessorGenerator {
public:
    SuccessorGenerator(const PetriNet& net, bool is_game = false, bool is_safety = false);
    SuccessorGenerator(const PetriNet& net, std::vector<std::shared_ptr<PQL::Condition> >& queries, bool is_game = false, bool is_safety = false);
    virtual ~SuccessorGenerator();
    void prepare(const MarkVal* state);
    bool next(MarkVal* write, PetriNet::player_t player = PetriNet::ANY);
    uint32_t fired()
    {
        return _suc_tcounter -1;
    }
        
    const MarkVal* parent() const {
        return _parent;
    }

    void reset();
    
protected:
    /**
     * Checks if the conditions are met for fireing t, if write != NULL,
     * then also consumes tokens from write while checking
     * @param t, transition to fire
     * @param write, marking to consume from (possibly NULL)
     * @return true if t is fireable, false otherwise
     */
    bool checkPreset(uint32_t t);

    /**
     * Consumes tokens in preset of t without from marking write checking
     * @param write, a marking to consume from
     * @param t, a transition to fire
     */
    void consumePreset(MarkVal* write, uint32_t t);

    /**
     * Produces tokens in write, given by t
     * @param write, a marking to produce to
     * @param t, a transition to fire
     */
    void producePostset(MarkVal* write, uint32_t t);
protected:
    const PetriNet& _net;
    const MarkVal* _parent;
    uint32_t _suc_pcounter;
    uint32_t _suc_tcounter;
    bool _is_game = false;
    bool _is_safety = false;

    friend class ReducingSuccessorGenerator;
};
}

#endif /* SUCCESSORGENERATOR_H */

