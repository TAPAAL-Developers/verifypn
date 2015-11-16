#include "../CTLParser/CTLParser.h"
#include "marking.h"
#include "edge.h"
#include "configuration.h"

namespace ctl{

bool Configuration::operator ==(const Configuration& rhs) const{
    if(this->Query() != rhs.Query())
        return false;
    else if(this->assignment != rhs.assignment)
        return false;
    else if(this->Marking() == rhs.Marking())
        return true;
    else
        return false;
}

}
