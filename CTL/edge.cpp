#include "edge.h"
#include "configuration.h"

namespace ctl{

//    bool Edge::operator ==(const Edge& rhs) const {
//        if(source != rhs.source)
//            return false;
//        if(!(targets == rhs.targets))
//            return false;
        
//        return true;
//    }



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

void Edge::rateEdge(){

    if(Rating > -1){
        return;
    }
    int alpha = 1;
    int beta = 1;
    int gamma = 1;

    int dist = source->query->Depth;
    int breath = 0;
    int succ = source->Successors.size();

    for(auto t : targets){
        breath++;
    }
    //make equation
    Rating = (alpha*dist) + (beta*breath) + (gamma*succ);
}

}
//namespace std{
//    // Specializations of hash functions.
//    // Normal
//    template<>
//    struct hash<ctl::Edge>{
//        size_t operator()(const ctl::Edge& t_edge ) const {
//            size_t seed = 0x9e3779b9;
//            hash<ctl::Configuration> hasher;
//            size_t result = hasher.operator()(*t_edge.source);

//            for(auto t : t_edge.targets){
//                result ^= hasher.operator()(*t);
//            }
//            result ^= seed + 0x9e3779b9 + (seed << 6) + (seed >> 2);

//            return result;
//        }
//    };
//    //
//    template<>
//    struct hash<ctl::Edge*>{
//        size_t operator()(const ctl::Edge* t_edge ) const {
//            hash<ctl::Edge> hasher;
//            return hasher.operator ()(*t_edge);
//        }
//    };
//}
