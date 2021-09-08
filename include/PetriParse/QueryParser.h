
#ifndef QUERYPARSER_H
#define QUERYPARSER_H

#include "PetriEngine/PQL/PQL.h"

struct query_item_t {
    std::string _id; // query name
    PetriEngine::PQL::Condition_ptr _query;

    enum {
        PARSING_OK,
        UNSUPPORTED_QUERY,
    } _parsing_result;
};

#endif /* QUERYPARSER_H */
