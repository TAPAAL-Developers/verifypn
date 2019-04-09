#ifndef STSOLVER_H
#define STSOLVER_H

#include "ResultPrinter.h"

#include "../lpsolve/lp_lib.h"

#include <memory>
#include <chrono>

namespace PetriEngine {
    class STSolver {
    
    struct STVariable {
        STVariable(int c, REAL v){
            colno=c;
            value=v;
        }
        int colno;
        REAL value;
    };
    
    struct place_t {
        uint32_t pre, post;
    };
        
    public:
        STSolver(ResultPrinter& printer, const PetriNet& net, PQL::Condition * query, uint32_t depth);
        virtual ~STSolver();
        int CreateFormula();
        int Solve(uint32_t timeout);
        void PrintStatistics();
        ResultPrinter::Result PrintResult();
        int getResult(){
            return _ret;
        }
        uint32_t getAnalysisTime(){
            return _analysisTime;
        }
        
    private:        
        std::string VarName(uint32_t index);
        void MakeConstraint(std::vector<STVariable> constraint, int constr_type, REAL rh);
        int CreateSiphonConstraints();
        int CreateStepConstraints(uint32_t i);
        int CreatePostVarDefinitions(uint32_t i);
        int CreateNoTrapConstraints();
        void constructPrePost();
        uint32_t duration() const;
        bool timeout() const;
        
        ResultPrinter& printer;
        PQL::Condition * _query;
        std::unique_ptr<place_t[]> _places;
        std::unique_ptr<uint32_t> _transitions;
        const PetriNet& _net;
        const MarkVal* _m0;
        lprec* _lp;
        uint32_t _siphonDepth;
        uint32_t _nPlaceVariables;
        uint32_t _nCol;
        int _ret = 0;     
        uint32_t _timelimit;
        uint32_t _analysisTime;
        int _buildTime;
        bool _lpBuilt;
        bool _solved;
        int _noTrap = 0;
        std::chrono::high_resolution_clock::time_point _start;
    };
}
#endif /* STSOLVER_H */

