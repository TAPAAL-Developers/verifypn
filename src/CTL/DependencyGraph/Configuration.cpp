#include "CTL/DependencyGraph/Configuration.h"

namespace DependencyGraph {

void Configuration::add_dependency(Edge *e) {
    if (_assignment == ONE)
        return;
    unsigned int sDist =
        e->_is_negated ? e->_source->get_distance() + 1 : e->_source->get_distance();
    unsigned int tDist = get_distance();

    set_distance(std::max(sDist, tDist));
    auto it = _dependency_set.begin();
    auto pit = _dependency_set.before_begin();
    while (it != _dependency_set.end()) {
        if (*it == e)
            return;
        if (*it > e)
            break;
        pit = it;
        ++it;
    }
    _dependency_set.insert_after(pit, e);
    ++e->_refcnt;
}
} // namespace DependencyGraph