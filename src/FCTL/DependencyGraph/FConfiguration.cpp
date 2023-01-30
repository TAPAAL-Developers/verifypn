#include "FCTL/DependencyGraph/FConfiguration.h"

namespace Featured {
    namespace DependencyGraph {

        void Configuration::addDependency(Edge* e) {
            if (assignment == ONE) return;
            unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
            unsigned int tDist = getDistance();

            setDistance(std::max(sDist, tDist));
            auto it = dependency_set.begin();
            auto pit = dependency_set.before_begin();
            while (it != dependency_set.end()) {
                if (*it == e) return;
                if (*it > e) break;
                pit = it;
                ++it;
            }
            dependency_set.insert_after(pit, e);
            ++e->refcnt;
        }
        std::string to_string(Assignment a)
        {
            switch (a) {

                case ONE:
                    return "ONE";
                    break;
                case UNKNOWN:
                    return "UNKNOWN";
                    break;
                case ZERO:
                    return "ZERO";
                    break;
                case CZERO:
                    return "CZERO";
                    break;
            }
        }
    }
}