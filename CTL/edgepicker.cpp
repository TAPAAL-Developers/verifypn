#include "edgepicker.h"

namespace ctl {

Edge* EdgePicker::pop(){
    if(_strategy == CTL_DFS){
        return DFS();
    }
    else if (_strategy == CTL_BFS){
        return BFS();
    }
}

void EdgePicker::push(Edge *t_edge){
    if(_strategy == CTL_DFS){
        DFS(t_edge);
    }
    else if (_strategy == CTL_BFS){
        BFS(t_edge);
    }
}

//DFS pop
inline Edge* EdgePicker::DFS(){
    Edge* e = W.front();
    W.pop_front();
    return e;
}
//DFS push
inline void EdgePicker::DFS(Edge* t_edge){
    W.push_front(t_edge);
}
//BFS pop
inline Edge* EdgePicker::BFS(){
    Edge* e = W.front();
    W.pop_front();
    return e;
}
//BFS push
inline void EdgePicker::BFS(Edge* t_edge){
    W.push_back(t_edge);
}


}
