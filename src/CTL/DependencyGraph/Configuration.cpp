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
            if(*it == e) {
#ifdef DG_REFCOUNTING
                assert(std::find(e->source->forward_dependency_set.begin(),
                                 e->source->forward_dependency_set.end(),
                                 this) != e->source->forward_dependency_set.end());
#endif
                return;
            }
            if(*it == e) return;
            pit = it;
            ++it;
        }
        dependency_set.insert_after(pit, e);
#ifdef DG_REFCOUNTING
        assert(std::find(
                std::begin(e->source->forward_dependency_set), std::end(e->source->forward_dependency_set), this) == std::en
d(forward_dependency_set));

        e->source->forward_dependency_set.emplace_front(this);

# ifndef NDEBUG
        size_t nelem = 0;
        for (auto &_: dependency_set) {
            ++nelem;
        }
        assert(refc >= nelem);
        assert((refc == 0) == (dependency_set.empty()));
# endif
#endif
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

#ifdef DG_REFCOUNTING
    void Configuration::remove_dependent(Configuration *c) {
        assert(refc > 0);
        assert(std::find_if(
                std::begin(dependency_set), std::end(dependency_set),
                [&](auto e) { return e->source == c; }) != std::end(dependency_set));
        {
            auto pit = dependency_set.before_begin();
            auto it = dependency_set.begin();
            while (it != dependency_set.end()) {
                if ((*it)->source == c) {
                    dependency_set.erase_after(pit);
                    --refc;
                    it = pit;
                }
                pit = it;
                ++it;

/*{
            // might not be needed,
            auto pit = c->forward_dependency_set.before_begin();
            auto it = c->forward_dependency_set.begin();
            while (it != c->forward_dependency_set.end()) {
                if ((*it) == this) {
                    c->forward_dependency_set.erase_after(pit);
                    //--refc;
                    it = pit;
                }
                pit = it;
                ++it;
            }
        }*/
        assert(std::find_if(
                std::begin(dependency_set), std::end(dependency_set),
                [&](auto e) { return e->source == c; }) == std::end(dependency_set));

        assert((refc == 0) == (dependency_set.empty()));
    }
#endif
}