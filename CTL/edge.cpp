#include "edge.h"
#include "configuration.h"

namespace ctl{

    bool Edge::operator ==(const Edge& rhs) const {
        if(source != rhs.source)
            return false;
        return (targets == rhs.targets);
    }

    void Edge::edgePrinter(){
    	std::cout << "--------------- Edge Information -------------------\n";
	    std::cout << "--------------- source config----------------------\n";
	    	source->configPrinter();
        std::cout << "--------------- target configs----------------------\n";
	    for( auto c : targets){
	    	c->configPrinter();
        }
        std::cout << "-------------------------------------------------------\n";
    }
}
