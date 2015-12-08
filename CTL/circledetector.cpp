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
        _configs.insert(e->source);
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

    _path.push_back(e);

    if(circleDetected){
        circles++;
        if(isEvilCircle(*iter)){
//            std::cout << "Evil circle detected\n" << std::flush;
            (*iter)->assignment = CZERO;
            evilCircles++;
            return true;
        }
    }

    return false;
}

bool CircleDetector::push2(Edge *e)
{
    std::cout << "size of _path: " << _path.size() << std::endl << std::flush;
    std::cout << "size of _config: " << _path.size() << std::endl << std::flush;
    CTLTree *e_source_query = e->source->query;
    Configuration *s_config = e->source;

    e->source->configPrinter();

    if(verify_consistency())
        std::cout << "entered consistent\n" << std::flush;
    else
        std::cout << "entered inconsistent\n" << std::flush;

    //if path is empty
    if(_path.empty()){
        std::cout << "path is empty\n" << std::flush;
        current_query = e->source->query;
        if(e_source_query->quantifier == A && (e_source_query->path == F || e_source_query->path == U)){
            _path.push_back(e);

            //TODO remove later
            if(!_configs.empty()){
                std::cout << "Consistency Error: set of configs was not empty\n" << std::flush;
                exit(EXIT_FAILURE);
            }

            _configs.insert(s_config);
        }
        if(!verify_consistency())
            std::cout << "exiting: inconsistent\n" << std::flush;
        return false;
    }

    bool IsSub = isSubQuery(e_source_query, current_query);

    if(IsSub){
        std::cout << "IsSub: true \n" << std::flush;
        if(e_source_query->quantifier == A && (e_source_query->path == F || e_source_query->path == U)){
            _configs.insert(e->source);
            _path.push_back(e);
        }
        current_query = e_source_query;
        if(!verify_consistency())
            std::cout << "exiting: inconsistent\n" << std::flush;
        return false;
    }
    else if(e_source_query == current_query){
        std::cout << "current query equals source query\n" << std::flush;
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
                if(!verify_consistency())
                    std::cout << "exiting: inconsistent\n" << std::flush;
                return isEvil;
            }
        }
        if(!verify_consistency())
            std::cout << "exiting: inconsistent\n" << std::flush;
        return false;
    }
    else{
        std::cout << "source query is greater than current\n" << std::flush;
        std::cout << "Quantifier: " << e_source_query->quantifier << " Path: " << e_source_query->path << std::endl << std::flush;
        if(e_source_query->quantifier == A && (e_source_query->path == F || e_source_query->path == U)){
            std::cout << "new path\n" << std::flush;
            newPath(e);
            _configs.insert(e->source);
            _path.push_back(e);
        }
        else{
            std::cout << "new pseudo path\n" << std::flush;
            newPseudoPath(e);
        }
        current_query = e_source_query;
        if(!verify_consistency())
            std::cout << "exiting: inconsistent\n" << std::flush;
        return false;
    }
}

void CircleDetector::newPath(Edge *e)
{
    vector_iter iter = --(_path.end());

    while((*iter)->source != e->source && iter != _path.begin()){
        _configs.erase((*iter)->source);
        --iter;
    }
    _path.erase(iter, _path.end());
}

void CircleDetector::newPseudoPath(Edge *e)
{
    CTLTree *newQuery = e->source->query;
    vector_iter iter = --(_path.end());
    CTLTree *subquery = (*iter)->source->query;
    bool IsSub = isSubQuery(subquery, newQuery);

    auto citer =_configs.find(e->source);
    if(citer != _configs.end()){
        std::cout << "config already exists\n" << std::flush;
    }

    while(iter != _path.begin()){
        if(IsSub){
            int i =_configs.erase((*iter)->source);
            if(i == 0){
                std::cout << "Path Error2: Config in path but not in unordered set\n";
                exit(EXIT_FAILURE);
            }
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



bool CircleDetector::isEvilCircle(Configuration *t_config)
{
    vector_riter target = _path.rbegin();

    bool isEvil = true;
    int i = 0;
    while((*target)->source != t_config){
        if((*target)->source->Successors.size() > 1){
            isEvil = false;
            break;
        }
        target++;
    }
    return isEvil;
}

bool CircleDetector::verify_consistency()
{
    bool consistency = true;
    if(_configs.size() != _path.size()){
        std::cout << "Consistency Error: Inconsistent _configs " << _configs.size() << " and _path sizes " << _path.size() << std::endl;
        consistency = false;
        //return false;
    }

    for(Edge *e : _path){
        auto iter = _configs.find(e->source);
        if(iter == _configs.end()){
            std::cout << "Consistency Error: Configuration in path does not exists in _configs\n";
            consistency = false;
            //return false;
        }
    }
    return consistency;
}

}
