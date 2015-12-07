#include "circledetector.h"

namespace ctl {

CircleDetector::CircleDetector()
{
}

bool CircleDetector::push(Edge *e)
{
    //std::cout << "Max size of Path: " << _path.capacity() << std::endl << std::flush;
    //std::cout << "Size of Path: " << _path.size() << std::endl << std::flush;
    //e->edgePrinter();
    auto pair = _configs.insert(e->source);

    //Detect new path
    if(!pair.second){
        Edge* source_e = _path.back();
        _path.pop_back();
        // Pop all edges, until edge with same
        // source is found
        //std::cout << "creating new path" << std::endl << std::flush;
        while(source_e->source != e->source){
            auto erased = _configs.erase(source_e->source);
            //std::cout << ++i <<" removed config: " << erased << std::endl << std::flush;
            //source_e->source->configPrinter();
            source_e = _path.back();
            _path.pop_back();
        }
        //std::cout << "new path created"  << std::endl << std::flush;
        _path.push_back(e);
        return false;
    }
    else if(_path.empty() || (e->source == e->targets.front() && e->targets.size() == 1)){
        //std::cout << "Path was empty" << std::endl;
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
        if(get_trace(*iter)){
            evilCircles++;
            //std::cout << "Circle is evil\n";
            (*iter)->assignment == CZERO;
        }
    }

    _path.push_back(e);

    return false;
}

bool CircleDetector::get_trace(Configuration *t_source)
{
    //std::cout << "get trace" << std::endl;
    //std::list<Edge*> result;
    vector_riter target = _path.rbegin();
    //_path.pop_back();
    //result.push_front(target);

    bool isEvil = true;
    int i = 0;

//    std::cout << "Circle::Enter Loop" << std::endl << std::flush;
//    std::cout << "Circle::Size of _path:" << _path.size() << std::endl << std::flush;
    while((*target)->source != t_source){
        //std::cout << "trace is: " << ++i << std::endl << std::flush;
        //target->edgePrinter();
        if((*target)->source->Successors.size() > 1){
            isEvil = false;
            break;
        }

//        std::cout << "pre assignment" << std::endl << std::flush;
        target++;
//        std::cout << "post assignment" << std::endl << std::flush;
        //_path.pop();
        //result.push_front(target);
    }
//    std::cout << "Circle::Exit Loop" << std::endl;

//    for(Edge* e : result){
//        _path.push(e);
//    }
//    std::cout << "get trace end" << std::endl;
    return isEvil;
}

}
