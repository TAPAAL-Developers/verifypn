#ifndef CIRCLEDETECTOR_H
#define CIRCLEDETECTOR_H

#include "configuration.h"
#include "edge.h"
#include "../CTLParser/CTLParser.h"
#include <list>
#include <unordered_set>
#include <stack>
#include <cmath>
#include <algorithm>

namespace ctl{

class CircleDetector
{
    struct pointer_equal_to{
        bool operator()(const ctl::Edge* lhs, const ctl::Edge* rhs) const {
            return lhs->source == rhs->source;
        }
        bool operator ()(const ctl::Configuration *lhs, const ctl::Configuration * rhs) const{
            return lhs == rhs;
        }
    };
    struct pointer_hash{
        size_t operator()(const ctl::Edge *e) const {
            static const std::size_t shift = (size_t)log2(1 + sizeof(Edge*));
            return (std::size_t)(e) >> shift;
        }
        size_t operator()(const ctl::Configuration *c) const {
            static const std::size_t shift = (size_t)log2(1 + sizeof(Edge*));
            return (std::size_t)(c) >> shift;
        }
    };

public:

    CircleDetector();
    //virtual ~CircleDetector(){};

    bool push(Edge* e);
    bool push2(Edge* e);
    void newPath(Edge *e);
    void newPseudoPath(Edge *e);
    bool isSubQuery(CTLTree *subquery, CTLTree *query);
    bool isEvilCircle(Configuration *t_config);

    CTLTree *current_query;
    int circles = 0;
    int evilCircles = 0;

private:
    bool verify_consistency();

    typedef std::vector<Edge*>::iterator vector_iter;
    typedef std::vector<Edge*>::reverse_iterator vector_riter;
    typedef std::unordered_set<Configuration*, CircleDetector::pointer_hash, CircleDetector::pointer_equal_to> configSet;

    std::vector<Edge*> _path;
    configSet _configs;
};
}
#endif // CIRCLEDETECTOR_H
