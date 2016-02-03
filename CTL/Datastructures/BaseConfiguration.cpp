#include "BaseConfiguration.h"


ctl::BaseConfiguration::~BaseConfiguration()
{
    iterator current = successors.begin();
    iterator end = successors.end();

    while(current != end){
        delete *current;
    }

    current = deleted_sucessors.begin();
    end = deleted_sucessors.end();

    while(current != end){
        delete *current;
    }
}

void ctl::BaseConfiguration::remove_successor(ctl::HyperEdge *t_successor)
{
    iterator current = successors.begin();
    iterator end = successors.end();

    while(current != end){
        if(t_successor == (*current)){
            deleted_sucessors.insert(deleted_sucessors.end(), *current);
            successors.erase(current);
            break;
        }
    }
}
