#include "edgepicker.h"
#include <algorithm>

namespace ctl {

Edge* EdgePicker::pop(){
    if(_strategy == CTL_DFS){
        return DFS();
    }
    if(_strategy == CTL_CDFS){
        return CDFS();
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
    else if(_strategy == CTL_CDFS){
        CDFS(t_edge);
    }
    else if (_strategy == CTL_BFS){
        BFS(t_edge);
    }
    else if (_strategy == CTL_BestFS){
        BestFS(t_edge);
    }
}

void EdgePicker::remove(Edge* t_edge){
    auto it=W.begin();
    auto itn=W.end();
    int i = 0;

    while(it!=itn) {
        if(*it == t_edge){
            W.erase(it);
            itn=W.end();
            it=W.begin()+i;
        } else {it++;}
    }
}

bool EdgePicker::find(Edge* t_edge){
  if(std::find(W.begin(), W.end(), t_edge) != W.end()) return true;
  else return false;   
}

void EdgePicker::print(){
    for (auto w : W){
        w->edgePrinter();
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

//CDFS pop
Edge *EdgePicker::CDFS()
{
    Edge* e = W.front();

    if(_detector.push(e)){
        e->source->assignment = CZERO;
        //std::cout << "circle detected" << std::endl;
    }

    W.pop_front();
    return e;
}
//CDFS push
void EdgePicker::CDFS(Edge *e)
{
    W.push_front(e);
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
}

//BestFS push
inline void EdgePicker::BestFS(Edge* t_edge){
    t_edge->rateEdge();

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
