#include "DependencyGraph.h"

namespace ctl{

DependencyGraph *DependencyGraph::initialize(CTLTree &t_query){
    if(_query != NULL)
        clear(false);

    _query = &t_query;

    return this;
}

}//ctl
