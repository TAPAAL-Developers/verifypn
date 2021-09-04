/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

#include "PetriEngine/PQL/Contexts.h"



namespace PetriEngine {
    namespace PQL {

        bool ColoredAnalysisContext::resolve_place(const std::string& place, std::unordered_map<uint32_t, std::string>& out)
        {
            auto it = _coloredPlaceNames.find(place);
            if (it != _coloredPlaceNames.end()) {
                out = it->second;
                return true;
            }
            return false;
        }

        bool ColoredAnalysisContext::resolve_transition(const std::string& transition, std::vector<std::string>& out)
        {
            auto it = _coloredTransitionNames.find(transition);
            if (it != _coloredTransitionNames.end()) {
                out = it->second;
                return true;
            }
            return false;
        }


        AnalysisContext::ResolutionResult AnalysisContext::resolve(const std::string& identifier, bool place)
        {
            ResolutionResult result;
            result._offset = -1;
            result._success = false;
            auto& map = place ? _placeNames : _transitionNames;
            auto it = map.find(identifier);
            if (it != map.end()) {
                result._offset = (int) it->second;
                result._success = true;
                return result;
            }
            return result;
        }

        uint32_t SimplificationContext::get_lp_timeout() const
        {
            return _lpTimeout;
        }

        double SimplificationContext::get_reduction_time()
        {
            // duration in seconds
            auto end = std::chrono::high_resolution_clock::now();
            return (std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count())*0.000001;
        }

        glp_prob* SimplificationContext::make_base_lp() const
        {
            if (_base_lp == nullptr)
                _base_lp = build_base();
            if (_base_lp == nullptr)
                return nullptr;
            auto* tmp_lp = glp_create_prob();
            glp_copy_prob(tmp_lp, _base_lp, GLP_OFF);
            return tmp_lp;
        }

        glp_prob* SimplificationContext::build_base() const
        {
            constexpr auto infty = std::numeric_limits<double>::infinity();
            if (timeout())
                return nullptr;

            auto* lp = glp_create_prob();
            if (lp == nullptr)
                return lp;

            const uint32_t nCol = _net.number_of_transitions();
            const int nRow = _net.number_of_places();
            std::vector<int32_t> indir(std::max<uint32_t>(nCol, nRow) + 1);

            glp_add_cols(lp, nCol + 1);
            glp_add_rows(lp, nRow + 1);
            {
                std::vector<double> col = std::vector<double>(nRow + 1);
                for (size_t t = 0; t < _net.number_of_transitions(); ++t) {
                    auto pre = _net.preset(t);
                    auto post = _net.postset(t);
                    size_t l = 1;
                    while (pre.first != pre.second ||
                           post.first != post.second) {
                        if (pre.first == pre.second || (post.first != post.second && post.first->_place < pre.first->_place)) {
                            col[l] = post.first->_tokens;
                            indir[l] = post.first->_place + 1;
                            ++post.first;
                        }
                        else if (post.first == post.second || (pre.first != pre.second && pre.first->_place < post.first->_place)) {
                            if(!pre.first->_inhibitor)
                                col[l] = -(double) pre.first->_tokens;
                            else
                                col[l] = 0;
                            indir[l] = pre.first->_place + 1;
                            ++pre.first;
                        }
                        else {
                            assert(pre.first->_place == post.first->_place);
                            if(!pre.first->_inhibitor)
                                col[l] = (double) post.first->_tokens - (double) pre.first->_tokens;
                            else
                                col[l] = (double) post.first->_tokens;
                            indir[l] = pre.first->_place + 1;
                            ++pre.first;
                            ++post.first;
                        }
                        ++l;
                    }
                    glp_set_mat_col(lp, t + 1, l - 1, indir.data(), col.data());
                    if (timeout()) {
                        std::cerr << "glpk: construction timeout" << std::endl;
                        glp_delete_prob(lp);
                        return nullptr;
                    }
                }
            }
            int rowno = 1;
            for (size_t p = 0; p < _net.number_of_places(); p++) {
                glp_set_row_bnds(lp, rowno, GLP_LO, (0.0 - (double) _marking[p]), infty);
                ++rowno;
                if (timeout()) {
                    std::cerr << "glpk: construction timeout" << std::endl;
                    glp_delete_prob(lp);
                    return nullptr;
                }
            }
            return lp;
        }
    }
}