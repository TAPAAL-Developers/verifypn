#ifndef EDGEPICKER_H
#define EDGEPICKER_H

#include <deque>

#include "edge.h"
#include "dgengine.h"

namespace ctl {

class EdgePicker
{
public:
    EdgePicker(Search_Strategy t_strategy) : _strategy(t_strategy){}

    Edge* pop();
    void push(Edge* t_edge);
    inline size_t size() {return W.size();}
    inline bool empty() {return W.empty();}

private:

    //Function used to implement DFS
    inline Edge* DFS();
    inline void DFS(Edge* e);

    //Functions used to implement BFS
    inline Edge* BFS();
    inline void BFS(Edge* e);

    Search_Strategy _strategy = LOCALSMOLKA;
    std::deque<Edge*> W;
};

}
#endif // EDGEPICKER_H
