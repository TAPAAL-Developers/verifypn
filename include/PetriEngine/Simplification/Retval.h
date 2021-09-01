#ifndef RETVAL_H
#define RETVAL_H
#include "LinearPrograms.h"

namespace PetriEngine {
    namespace Simplification {
        struct Retval {
            std::shared_ptr<PQL::Condition> _formula = nullptr;
            AbstractProgramCollection_ptr _lps;
            AbstractProgramCollection_ptr _neglps;
            
            Retval (const std::shared_ptr<PQL::Condition> formula, 
            AbstractProgramCollection_ptr&& lps1, 
            AbstractProgramCollection_ptr&& lps2)
            : _formula(formula), _lps(std::move(lps1)),  _neglps(std::move(lps2)) {
                assert(_lps);
                assert(_neglps);                
            }

            Retval (const std::shared_ptr<PQL::Condition> formula, 
            const AbstractProgramCollection_ptr& lps1, 
            const AbstractProgramCollection_ptr& lps2) 
            : _formula(formula), _lps(lps1), _neglps(lps2) {
                assert(_lps);
                assert(_neglps);
            }
            
            Retval (const std::shared_ptr<PQL::Condition> formula) 
            : _formula(formula) {
                _lps = std::make_shared<SingleProgram>();
                _neglps  = std::make_shared<SingleProgram>();
                assert(_lps);
                assert(_neglps);
            }
 
            
            Retval(const Retval&& other)
            : _formula(std::move(other._formula)), _lps(std::move(other._lps)), _neglps(std::move(other._neglps))
            {
                assert(_lps);
                assert(_neglps);                
            }
            
            Retval(Retval&& other) 
            : _formula(other._formula), _lps(std::move(other._lps)), _neglps(std::move(other._neglps))
            {
                assert(_lps);
                assert(_neglps);
            }

            Retval& operator=(Retval&& other) {
                _lps = std::move(other._lps);
                _neglps = std::move(other._neglps);
                _formula = std::move(other._formula);
                assert(_lps);
                assert(_neglps);                
                return *this;
            }
            
            Retval() {
                _lps = std::make_shared<SingleProgram>();
                _neglps  = std::make_shared<SingleProgram>();
                assert(_lps);
                assert(_neglps);
            }
            
            ~Retval(){
            }
            
        private:

        };
    }
}

#endif /* RETVAL_H */

