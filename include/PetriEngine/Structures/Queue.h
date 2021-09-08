/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Queue.h
 * Author: Peter G. Jensen
 *
 * Created on 30 March 2016, 21:12
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <memory>
#include <queue>
#include <random>
#include <stack>

#include "../PQL/PQL.h"
#include "StateSet.h"

namespace PetriEngine::Structures {
class Queue {
  public:
    Queue(StateSetInterface &states);
    virtual ~Queue();
    virtual auto pop(Structures::State &state) -> bool = 0;
    auto last_popped() -> size_t { return _last; }

  protected:
    StateSetInterface &_states;
    size_t _last = 0;
};

class BFSQueue : public Queue {
  public:
    BFSQueue(StateSetInterface &states);
    ~BFSQueue() override;

    auto pop(Structures::State &state) -> bool override;
    void push(size_t id);

  private:
    size_t _cnt;
    size_t _nstates;
};

class DFSQueue : public Queue {
  public:
    DFSQueue(StateSetInterface &states);
    ~DFSQueue() override;

    auto pop(Structures::State &state) -> bool override;
    auto top(Structures::State &state) const -> bool;
    void push(size_t id);

  private:
    std::stack<size_t> _stack;
};

class RDFSQueue : public Queue {
  public:
    RDFSQueue(StateSetInterface &states, size_t seed);
    ~RDFSQueue() override;

    auto pop(Structures::State &state) -> bool override;
    auto top(Structures::State &state) -> bool;
    void push(size_t id);

  private:
    std::stack<size_t> _stack;
    std::vector<size_t> _cache;
    std::default_random_engine _rng;
};

class HeuristicQueue : public Queue {
  public:
    struct weighted_t {
        size_t _weight;
        size_t _item;
        weighted_t(size_t w, size_t i) : _weight(w), _item(i){};
        auto operator<(const weighted_t &y) const -> bool {
            if (_weight == y._weight)
                return _item < y._item; // do dfs if they match
            //                    if(weight == y.weight) return item > y.item;// do bfs if they
            //                    match
            return _weight > y._weight;
        }
    };

    HeuristicQueue(StateSetInterface &states);
    ~HeuristicQueue() override;

    auto pop(Structures::State &state) -> bool override;
    void push(size_t id, uint32_t distance);

  private:
    std::priority_queue<weighted_t> _queue;
};
} // namespace PetriEngine::Structures

#endif /* QUEUE_H */
