#include "edge.h"

namespace ctl{

    bool Edge::operator ==(const Edge& rhs) const {
        if(Source != rhs.Source)
            return false;
        return (Targets == rhs.Targets);
    }

}
