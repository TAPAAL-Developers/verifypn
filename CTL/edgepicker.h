#ifndef EDGEPICKER_H
#define EDGEPICKER_H

#include <deque>

#include "circledetector.h"
#include "edge.h"
#include "dgengine.h"

namespace ctl {

class EdgePicker
{
public:
    EdgePicker(ctl_search_strategy t_strategy) : _strategy(t_strategy){}

    Edge* pop();
    void push(Edge* t_edge);
    void push_dependency(Edge *t_edge);
    void remove(Edge* t_edge);
    void reset();
    void restore();
    void print();
    bool find(Edge* t_edge);
    inline size_t size() {return W.size();}
    inline bool empty() {return W.empty();}

private:

    //Function used to implement DFS
    inline Edge* DFS();
    inline void DFS(Edge* e);

    //Functions used to implement BDFS
    inline Edge* BDFS();
    inline void BDFS(Edge* t_edge);
    inline void BDFS_dependency(Edge* t_edge);

    //Function used to implement DFS
    //With Circle Dectection
    inline Edge* CDFS();
    inline void CDFS(Edge* e);

    //Functions used to implement BFS
    inline Edge* BFS();
    inline void BFS(Edge* e);

    //Functions used to implement Forward BFS
    inline Edge* FBFS();
    inline void FBFS(Edge* t_edge);
    inline void FBFS_dependency(Edge* t_edge);

    //Functions used to implement Backward BFS
    inline Edge* BBFS();
    inline void BBFS(Edge* t_edge);
    inline void BBFS_dependency(Edge* t_edge);

    //Functions used to implement BFS
    inline Edge* BestFS();
    inline void BestFS(Edge* e);


    ctl::CircleDetector _detector;
    ctl_search_strategy _strategy;
    std::deque<Edge*> W;
    std::deque<Edge*> dW:
};

}
#endif // EDGEPICKER_H
