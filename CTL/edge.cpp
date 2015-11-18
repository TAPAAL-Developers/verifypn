#include "edge.h"

namespace ctl{

    bool Edge::operator ==(const Edge& rhs) const {
        if(source != rhs.source)
            return false;
        return (targets == rhs.targets);
    }

}
