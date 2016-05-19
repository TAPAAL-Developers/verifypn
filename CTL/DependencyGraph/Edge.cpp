#include "Edge.h"
#include "Configuration.h"

#include <sstream>

std::string DependencyGraph::Edge::toString(){
    std::stringstream ss;

    ss << "========================= Edge: Source =========================" << std::endl
       << source->toString()
       << "==========================Edge: Targets ========================" << std::endl;

    for(Configuration * c : targets){
        ss << c->toString();
    }

    ss << "================================================================" << std::endl;

    return ss.str();
}
