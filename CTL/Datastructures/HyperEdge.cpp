#include "HyperEdge.h"


inline void ctl::HyperEdge::add_target(ctl::BaseConfiguration *t_target)
{
    targets.insert(targets.end(), t_target);
}
