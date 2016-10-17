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

#include "SuccessorGenerator.h"
#include "Structures/State.h"
namespace PetriEngine {
    SuccessorGenerator::SuccessorGenerator(const PetriNet& net, const Structures::State& p ) 
    : _net(net), _parent(p) {
        _suc_pcounter = 0;
        _suc_tcounter = std::numeric_limits<uint32_t>::max();  
    }

    SuccessorGenerator::~SuccessorGenerator() {
    }
    
       
    bool SuccessorGenerator::next(Structures::State& write)
    {
        for (; _suc_pcounter < _net._nplaces; ++_suc_pcounter) {
            // orphans are currently under "place 0" as a special case
            if(_suc_pcounter == 0 || _parent.marking()[_suc_pcounter] > 0)
            {
                if(_suc_tcounter == std::numeric_limits<uint32_t>::max())
                {
                    _suc_tcounter = _net._placeToPtrs[_suc_pcounter];
                }
                uint32_t last = _net._placeToPtrs[_suc_pcounter+1];
                for(;_suc_tcounter != last; ++_suc_tcounter)
                {
                    memcpy(write.marking(), _parent.marking(), _net._nplaces*sizeof(MarkVal));
                    
                    const TransPtr& ptr = _net._transitions[_suc_tcounter];
                    uint32_t finv = ptr.inputs;
                    uint32_t linv = ptr.outputs;
                    bool ok = true;
                    
                    for(;finv < linv; ++finv)
                    {
                        const Invariant& inv = _net._invariants[finv];
                        if(_parent.marking()[inv.place] < inv.tokens)
                        {
                            if(!inv.inhibitor)
                            {
                                ok = false;
                                break;
                            }
                        }
                        else
                        {
                            if(inv.inhibitor)
                            {
                                ok = false;
                                break;
                            }
                            else
                            {
                                write.marking()[_net._invariants[finv].place] -= _net._invariants[finv].tokens;
                            }
                        }
                    }
                    
                    if(!ok) continue;
                    // else fire
                    finv = linv;
                    linv = _net._transitions[_suc_tcounter+1].inputs;
                    for(;finv < linv; ++finv)
                    {
                        size_t n = write.marking()[_net._invariants[finv].place];
                        n += _net._invariants[finv].tokens;
                        if(n >= std::numeric_limits<uint32_t>::max())
                        {
                            exit(-1);
                        }
                        write.marking()[_net._invariants[finv].place] = n;
                    }
                    ++_suc_tcounter;
                    return true;
                }
                _suc_tcounter = std::numeric_limits<uint32_t>::max();
            }
            _suc_tcounter = std::numeric_limits<uint32_t>::max();
        }
        return false;
    }  
}
