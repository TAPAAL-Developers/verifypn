#ifndef CTLRESULT_H
#define CTLRESULT_H

#include "PetriEngine/PQL/PQL.h"
#include "errorcodes.h"

#include <string>

struct CTLResult {
    CTLResult(const PetriEngine::PQL::Condition_ptr &qry) { _query = qry; }

    PetriEngine::PQL::Condition_ptr _query;
    bool _result;

    double _duration = 0;
    size_t _numberOfMarkings = 0;
    size_t _numberOfConfigurations = 0;
    size_t _processedEdges = 0;
    size_t _processedNegationEdges = 0;
    size_t _exploredConfigurations = 0;
    size_t _numberOfEdges = 0;
};

#endif // CTLRESULT_H
