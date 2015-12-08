#include "circledetector.h"

namespace ctl {

CircleDetector::CircleDetector()
{
}

bool CircleDetector::push(Edge *e)
{
    auto pair = _configs.insert(e->source);

    //Detect new path
    if(!pair.second){
        Edge* source_e = _path.back();
        _path.pop_back();
        while(source_e->source != e->source){
            auto erased = _configs.erase(source_e->source);
            source_e = _path.back();
            _path.pop_back();
        }
        _path.push_back(e);
        return false;
    }
    else if(_path.empty() || (e->source == e->targets.front() && e->targets.size() == 1)){
        _path.push_back(e);
        return false;
    }

    bool circleDetected = false;
    // Detect circles by looking through target set
    // if configuration is found, we have a circle
    configSet::const_iterator iter;
    for(Configuration* c : e->targets){
        iter = _configs.find(c);
        if(iter != _configs.end()){
            //(*iter)->configPrinter();
            circleDetected = true;
            //std::cout << "Circle::Found\n";
            break;
        }
    }

    if(circleDetected){
        circles++;
        if(isEvilCircle(*iter)){
            evilCircles++;
            (*iter)->assignment == CZERO;
        }
    }

    _path.push_back(e);

    return false;
}

bool CircleDetector::push2(Edge *e)
{
    CTLTree *source_query = e->source->query;
    Configuration *source = e->source;

    //Case: Base Case TODO Fix, it is wrong
    if(_path.empty() || (e->source == e->targets.front() && e->targets.size() == 1)){
        _path.push_back(e);
        _configs.insert(source);
        current_query = source_query;
    }
    else if(current_query == source_query){
        if(current_query->quantifier == A && (current_query->path == F || current_query->path == U)){
            auto pair = _configs.insert(source);
            if(pair.second){
                _path.push_back(e);
            }
            else{

            }
        }
        return false;
    }
}

void CircleDetector::newPath(Edge *e)
{
    Configuration *source = e->source;
    vector_iter iter = --(_path.end());

    //while(iter != _path.begin()){



    --iter;
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
