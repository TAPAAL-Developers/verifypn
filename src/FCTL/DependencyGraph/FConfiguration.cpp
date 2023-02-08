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
                case DONE:
                    return "DONE";
                    break;
            }
        }

        bool Configuration::remove_suc(Edge *e, bool del)
        {
            if (del) {
                assert(!del || std::find(successors.begin(), successors.end(), e) != successors.end());
                successors.remove(e);
//            if (it == successors.end()) {
//                assert(false);
//                return false;
//            }
                --e->refcnt;
                --nsuccs;
                assert(num_successors() == nsuccs);
                if (nsuccs == 0) {
                    assert(successors.empty());
                    return true;
                }
                return false;
            }
            else {
                --e->refcnt; --nsuccs;
                return nsuccs == 0;
            }
        }
    }
}
