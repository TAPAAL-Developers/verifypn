#include "circledetector.h"

namespace ctl {

bool CircleDetector::push(Edge *e)
{
    //std::cout << "Size of Path: " << _path.size() << std::endl;
    //e->edgePrinter();
    auto pair = _configs.insert(e->source);

    //Detect new path
    if(!pair.second){
        Edge* source_e = _path.top();
        _path.pop();
        // Pop all edges, until edge with same
        // source is found
        while(source_e->source != e->source){
            _configs.erase(source_e->source);
            source_e = _path.top();
            _path.pop();
        }

        _path.push(e);
        return false;
    }
    else if(_path.empty() || (e->source == e->targets.front() && e->targets.size() == 1)){
        //std::cout << "Path was empty" << std::endl;
        _path.push(e);
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
//            std::cout << "Circle::Found" << std::endl;
            break;
        }
    }

    if(circleDetected){
        if(get_trace(*iter)){
//            std::cout << "Circle is evil" << std::endl;
            (*iter)->assignment == CZERO;
        }
    }

    _path.push(e);

    return false;
}

bool CircleDetector::get_trace(Configuration *t_source)
{
    //std::cout << "get trace" << std::endl;
    std::list<Edge*> result;
    Edge *target = _path.top();
    _path.pop();
    result.push_front(target);

    bool isEvil = true;
    int i = 0;

//    std::cout << "Circle::Enter Loop" << std::endl << std::flush;
//    std::cout << "Circle::Size of _path:" << _path.size() << std::endl << std::flush;
    while(target->source != t_source){
//        std::cout << "trace is: " << ++i << std::endl << std::flush;
        //target->edgePrinter();
        if(target->source->Successors.size() > 1){
            isEvil = false;
        }

//        std::cout << "pre assignment" << std::endl << std::flush;
        target = _path.top();
//        std::cout << "post assignment" << std::endl << std::flush;
        _path.pop();
        result.push_front(target);
    }
//    std::cout << "Circle::Exit Loop" << std::endl;

    for(Edge* e : result){
        _path.push(e);
    }
//    std::cout << "get trace end" << std::endl;
    return isEvil;
}

}
