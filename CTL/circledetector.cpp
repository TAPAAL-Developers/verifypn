#include "circledetector.h"
#include "dgengine.h"

namespace ctl {

CircleDetector::CircleDetector()
{
}

bool CircleDetector::push(Edge *e)
{
    auto pair = _configs.insert(e->source);

//    std::cout << "===================================================\n";
//    e->source->configPrinter();
//    std::cout << "===================================================\n";

    //Detect new path
    if(!pair.second){
//        std::cout << "config we are looking for\n";
//        (*pair.first)->configPrinter();
//        std::cout << "New path detected\n" << std::flush;
        Edge* source_e = _path.back();
//        std::cout << "size of _path: " << _path.size() << std::endl << std::flush;
        _path.pop_back();

        while(source_e->source != e->source){
            auto erased = _configs.erase(source_e->source);
//            std::cout << "size of _path: " << _path.size() << std::endl << std::flush;
            source_e = _path.back();
            _path.pop_back();
        }
//        std::cout << "Done popping old path\n" << std::flush;
        _path.push_back(e);
        return false;
    }
    else if(_path.empty() || (e->source == e->targets.front() && e->targets.size() == 1)){
//        std::cout << "Funny case\n"  << std::flush;
        _path.push_back(e);
        return false;
    }
//    else
//        std::cout << "by passed first if-else chain\n"  << std::flush;

    bool circleDetected = false;
    // Detect circles by looking through target set
    // if configuration is found, we have a circle
    configSet::const_iterator iter;
    for(Configuration* c : e->targets){
        iter = _configs.find(c);
        if(iter != _configs.end()){
//            (*iter)->configPrinter();
            circleDetected = true;
//            std::cout << "Circle::Found\n";
            break;
        }
    }

    if(circleDetected){
        circles++;
        if(isEvilCircle(*iter)){
//            std::cout << "Evil circle detected\n" << std::flush;
            (*iter)->assignment = CZERO;
            evilCircles++;
            _path.push_back(e);
            return true;
        }
    }

    _path.push_back(e);

    return false;
}

bool CircleDetector::push2(Edge *e)
{
    CTLTree *s_query = e->source->query;
    Configuration *s_config = e->source;

    //auto s_source_iter = _configs.find(e->source);
    CTLTree *e_source_query = e->source->query;
    bool IsSub = isSubQuery(current_query, e_source_query);

    if(IsSub){
        if(e_source_query->quantifier == A && (e_source_query->path == F || e_source_query->path == U)){
            _configs.insert(e->source);
            _path.push_back(e);
        }
        current_query = e_source_query;
        return false;
    }
    else if(e_source_query == current_query){
        if(e_source_query->quantifier == A && (e_source_query->path == F || e_source_query->path == U)){
            auto pair = _configs.insert(e->source);

            if(!pair.second){
                newPath(e);
                _configs.insert(e->source);
                _path.push_back(e);
            }
            else{
                //circle detection part
                bool isEvil = false;
                for(Configuration *c : e->targets){
                    if(_configs.find(c) != _configs.end()){
                        circles++;
                        if(isEvilCircle(c)){
                            evilCircles++;
                            isEvil = true;
                            c->assignment = CZERO;
                            break;
                        }
                    }
                }
                _path.push_back(e);
                return isEvil;
            }
        }
        return false;
    }
    else{
        if(e_source_query->quantifier == A && (e_source_query->path == F || e_source_query->path == U)){
            newPath(e);
            _configs.insert(e->source);
            _path.push_back(e);
        }
        else{
            newPseudoPath(e);
        }
        current_query = e_source_query;
        return false;
    }
}

void CircleDetector::newPath(Edge *e)
{
    vector_iter iter = --(_path.end());

    while((*iter)->source != e->source){
        _configs.erase((*iter)->source);
        --iter;
    }
    _path.erase(--iter, _path.end());
}

void CircleDetector::newPseudoPath(Edge *e)
{
    CTLTree *newQuery = e->source->query;
    vector_iter iter = --(_path.end());
    CTLTree *subquery = (*iter)->source->query;
    bool IsSub = isSubQuery(subquery, newQuery);

    while(iter != _path.begin()){
        if(IsSub){
            _configs.erase((*iter)->source);
            --iter;
            if((*iter)->source->query != subquery){
                subquery = (*iter)->source->query;
                IsSub = isSubQuery(subquery, newQuery);
            }
        }
        else
            break;
    }

    if(iter == _path.begin()){
        if(IsSub){
            _path.pop_back();
            int i = _configs.erase((*iter)->source);
            if(i == 0){
                std::cout << "Path Error: Config in path but not in unordered set\n";
                exit(EXIT_FAILURE);
            }
        }
    }
}

bool CircleDetector::isSubQuery(CTLTree *subquery, CTLTree *query)
{
    if(subquery == query)
        return false;

    std::deque<CTLTree*> queue;
    queue.push_back(query);

    while(!queue.empty()){
        CTLTree *q = queue.front();
        queue.pop_front();
        Quantifier qq = q->quantifier;
        Path qp = q->path;

        if(q == subquery){
            return true;
        }
        //no descandents
        else if(qq == EMPTY){
        }
        //two descandants
        else if(qp == U || qq == OR || qq == AND){
            queue.push_back(q->first);
            queue.push_back(q->second);
        }
        //One descendants
        else {
            queue.push_back(q->first);
        }
    }
    return false;
}



bool CircleDetector::isEvilCircle(Configuration *t_source)
{
    vector_riter target = _path.rbegin();

    bool isEvil = true;
    int i = 0;
    while((*target)->source != t_source){
        if((*target)->source->Successors.size() > 1){
            isEvil = false;
            break;
        }
        target++;
    }
    return isEvil;
}

}
