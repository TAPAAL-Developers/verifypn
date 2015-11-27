#include "edgepicker.h"

namespace ctl {

Edge* EdgePicker::pop(){
    if(_strategy == LOCALSMOLKA){
        return DFS();
    }
    else if (_strategy == LOCALSMOLKA_BFS){
        return BFS();
    }
    else if (_strategy == GLOBALSMOLKA){
        return DFS();
    }
    else if (_strategy == GLOBALSMOLKA_BFS){
        return BFS();
    }
}

void EdgePicker::push(Edge *t_edge){
    if(_strategy == LOCALSMOLKA){
        DFS(t_edge);
    }
    else if (_strategy == LOCALSMOLKA_BFS){
        BFS(t_edge);
    }
    else if (_strategy == GLOBALSMOLKA){
        DFS(t_edge);
    }
    else if (_strategy == GLOBALSMOLKA_BFS){
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
