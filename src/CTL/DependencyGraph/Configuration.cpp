#include "CTL/DependencyGraph/Configuration.h"


namespace DependencyGraph {

    void Configuration::addDependency(Edge* e) {
        if(assignment == ONE) return;
        unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
        unsigned int tDist = getDistance();

        setDistance(std::max(sDist, tDist));
        auto it = dependency_set.begin();
        auto pit = dependency_set.before_begin();
        while(it != dependency_set.end())
        {
            if(*it == e) return;
            pit = it;
            ++it;
        }
        dependency_set.insert_after(pit, e);
        ++e->refcnt;
    }

#ifndef NDEBUG
    bool Edge::has_suc(size_t id) {
        for (auto *conf : targets) {
            if (conf->id == id) {
                return true;
            }
        }
        return false;
    }
#endif
}