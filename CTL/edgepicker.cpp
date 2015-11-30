#include "edgepicker.h"
#include <algorithm> 

namespace ctl {

Edge* EdgePicker::pop(){
    if(_strategy == CTL_DFS){
        return DFS();
    }
    else if (_strategy == CTL_BFS){
        return BFS();
    }
    else if (_strategy == CTL_BestFS){
        return BestFS();
    }
}

void EdgePicker::push(Edge *t_edge){
    if(_strategy == CTL_DFS){
        DFS(t_edge);
    }
    else if (_strategy == CTL_BFS){
        BFS(t_edge);
    }
    else if (_strategy == CTL_BestFS){
        return BestFS(t_edge);
    }
}

void EdgePicker::remove(Edge* t_edge){
    if(std::find(W.begin(), W.end(), t_edge) != W.end()){
        int i = 0;
        for (auto e : W){
            if(e == t_edge)
                W.erase(W.begin() + i);
            i++;
        }
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


//BestFS pop
inline Edge* EdgePicker::BestFS(){
    Edge* e = W.front();
    W.pop_front(); 
    return e; 
   /*Edge* bestEdge = W.front();
    int i = 0;
    int lowestNR = bestEdge->targets.size();

    for (auto e : W){
        if(e->targets.size() < lowestNR){
            if(lowestNR == 1){
                W.erase(W.begin() + i);
                return e;
            } 
        }
        i++;
        if(i > 10){
            break;
        }
    }
    Edge* e = W.front();
    W.pop_front();
    return e;*/


}
//BestFS push
inline void EdgePicker::BestFS(Edge* t_edge){
    t_edge->rateEdge();
    W.push_back(t_edge);

    auto it = W.begin();

    while(it != W.end()){
        if(t_edge->Rating < (*it)->Rating){
            W.insert(it, 1, t_edge);
            return;
        }
        else {
            it++;
        }
    }

    W.push_back(t_edge);
}



}
