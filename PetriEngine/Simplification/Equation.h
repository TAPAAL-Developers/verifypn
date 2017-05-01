#ifndef EQUATION_H
#define EQUATION_H
#include "Member.h"

namespace PetriEngine {
    namespace Simplification {
        class Equation {
        public:
            Equation(Member& lh, Member& rh, std::string op) : op(op){
                // put variables on left hand side
                int size = std::max(lh.variables.size(), rh.variables.size());
                row.resize(size);
                lh.variables.resize(size, 0);
                rh.variables.resize(size, 0);
                std::transform(lh.variables.begin(),lh.variables.end(),rh.variables.begin(),row.begin(),std::minus<int>());

                // put constant on right hand side
                constant = rh.constant - lh.constant; 
            }
            Equation(const Equation& eq) : row(eq.row), op(eq.op), constant(eq.constant) {
            }      
            Equation(){
            }

            std::vector<int> row;
            std::string op;
            int constant;
            virtual ~Equation(){}
        };
    }
}

#endif /* EQUATION_H */
