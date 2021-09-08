#include <cassert>
#include <cmath>
#include <fstream>
#include <glpk.h>

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Simplification/LinearProgram.h"

namespace PetriEngine {
namespace Simplification {
using REAL = double;
LinearProgram::~LinearProgram() {}

LinearProgram::LinearProgram(const std::vector<int> &data, int constant, op_t op) {
    // TODO fix memory-management here!
    equation_t c(data);
    switch (op) {
    case Simplification::OP_LT:
        c._upper = constant - 1;
        break;
    case Simplification::OP_GT:
        c._lower = constant + 1;
        break;
    case Simplification::OP_LE:
        c._upper = constant;
        break;
    case Simplification::OP_GE:
        c._lower = constant;
        break;
    case Simplification::OP_EQ:
        c._lower = constant;
        c._upper = constant;
        break;
    default:
        // We ignore this operator for now by not adding any equation.
        // This is untested, however.
        assert(false);
    }
    _equations.emplace_back(c);
}

constexpr auto infty = std::numeric_limits<REAL>::infinity();

bool LinearProgram::is_impossible(const PQL::SimplificationContext &context, uint32_t solvetime) {
    bool use_ilp = true;
    auto &net = context.net();

    if (_result != result_t::UKNOWN) {
        if (_result == result_t::IMPOSSIBLE)
            return _result == result_t::IMPOSSIBLE;
    }

    if (_equations.size() == 0 || context.timeout()) {
        return false;
    }

    const uint32_t nCol = net.number_of_transitions();
    const uint32_t nRow = net.number_of_places() + _equations.size();

    std::vector<REAL> row = std::vector<REAL>(nCol + 1);
    std::vector<int32_t> indir(std::max(nCol, nRow) + 1);
    for (size_t i = 0; i <= nCol; ++i)
        indir[i] = i;

    auto lp = context.make_base_lp();
    if (lp == nullptr)
        return false;

    int rowno = 1 + net.number_of_places();
    glp_add_rows(lp, _equations.size());
    for (const auto &eq : _equations) {
        auto l = eq._row.write_indir(row, indir);
        assert(!(std::isinf(eq._upper) && std::isinf(eq._lower)));
        glp_set_mat_row(lp, rowno, l - 1, indir.data(), row.data());
        if (!std::isinf(eq._lower) && !std::isinf(eq._upper)) {
            if (eq._lower == eq._upper)
                glp_set_row_bnds(lp, rowno, GLP_FX, eq._lower, eq._upper);
            else {
                if (eq._lower > eq._upper) {
                    _result = result_t::IMPOSSIBLE;
                    glp_delete_prob(lp);
                    return true;
                }
                glp_set_row_bnds(lp, rowno, GLP_DB, eq._lower, eq._upper);
            }
        } else if (std::isinf(eq._lower))
            glp_set_row_bnds(lp, rowno, GLP_UP, -infty, eq._upper);
        else
            glp_set_row_bnds(lp, rowno, GLP_LO, eq._lower, -infty);
        ++rowno;

        if (context.timeout()) {
            std::cerr << "glpk: construction timeout" << std::endl;
            glp_delete_prob(lp);
            return false;
        }
    }

    // Set objective, kind and bounds
    for (size_t i = 1; i <= nCol; i++) {
        glp_set_obj_coef(lp, i, 0);
        glp_set_col_kind(lp, i, use_ilp ? GLP_IV : GLP_CV);
        glp_set_col_bnds(lp, i, GLP_LO, 0, infty);
    }

    // Minimize the objective
    glp_set_obj_dir(lp, GLP_MIN);
    auto stime = glp_time();
    glp_smcp settings;
    glp_init_smcp(&settings);
    auto timeout = std::min(solvetime, context.get_lp_timeout()) * 1000;
    settings.tm_lim = timeout;
    settings.presolve = GLP_OFF;
    settings.msg_lev = 0;
    auto result = glp_simplex(lp, &settings);
    if (result == GLP_ETMLIM) {
        _result = result_t::UKNOWN;
        std::cerr << "glpk: timeout" << std::endl;
    } else if (result == 0) {
        auto status = glp_get_status(lp);
        if (status == GLP_OPT) {
            glp_iocp iset;
            glp_init_iocp(&iset);
            iset.msg_lev = 0;
            iset.tm_lim = std::max<uint32_t>(timeout - (stime - glp_time()), 1);
            iset.presolve = GLP_OFF;
            auto ires = glp_intopt(lp, &iset);
            if (ires == GLP_ETMLIM) {
                _result = result_t::UKNOWN;
                std::cerr << "glpk mip: timeout" << std::endl;
            } else if (ires == 0) {
                auto ist = glp_mip_status(lp);
                if (ist == GLP_OPT || ist == GLP_FEAS || ist == GLP_UNBND) {
                    _result = result_t::POSSIBLE;
                } else {
                    _result = result_t::IMPOSSIBLE;
                }
            }
        } else if (status == GLP_FEAS || status == GLP_UNBND) {
            _result = result_t::POSSIBLE;
        } else
            _result = result_t::IMPOSSIBLE;
    } else if (result == GLP_ENOPFS || result == GLP_ENODFS || result == GLP_ENOFEAS) {
        _result = result_t::IMPOSSIBLE;
    }
    glp_delete_prob(lp);

    return _result == result_t::IMPOSSIBLE;
}

std::vector<std::pair<double, bool>>
LinearProgram::bounds(const PQL::SimplificationContext &context, uint32_t solvetime,
                      const std::vector<uint32_t> &places) {
    std::vector<std::pair<double, bool>> result(
        places.size() + 1, std::make_pair(std::numeric_limits<double>::infinity(), false));
    auto &net = context.net();
    auto m0 = context.marking();
    auto timeout = std::min(solvetime, context.get_lp_timeout());

    const uint32_t nCol = net.number_of_transitions();
    std::vector<REAL> row = std::vector<REAL>(nCol + 1);

    glp_smcp settings;
    glp_init_smcp(&settings);
    settings.tm_lim = timeout * 1000;
    settings.presolve = GLP_OFF;
    settings.msg_lev = 0;

    for (size_t it = 0; it <= places.size(); ++it) {
        // we want to start with the overall bound, most important
        // Spend time on rest after
        auto stime = glp_time();
        size_t pi;
        if (it == 0)
            pi = places.size();
        else
            pi = it - 1;

        if (context.timeout()) {
            return result;
        }
        // Create objective
        memset(row.data(), 0, sizeof(REAL) * net.number_of_transitions() + 1);
        double p0 = 0;
        bool all_le_zero = true;
        bool all_zero = true;
        if (pi < places.size()) {
            auto tp = places[pi];
            p0 = m0[tp];
            for (size_t t = 0; t < net.number_of_transitions(); ++t) {
                row[1 + t] = net.out_arc(t, tp) - net.in_arc(tp, t);
                all_le_zero &= row[1 + t] <= 0;
                all_zero &= row[1 + t] == 0;
            }
        } else {
            for (size_t t = 0; t < net.number_of_transitions(); ++t) {
                double cnt = 0;
                for (auto tp : places)
                    cnt += net.out_arc(t, tp) - net.in_arc(tp, t);
                row[1 + t] = cnt;
                all_le_zero &= row[1 + t] <= 0;
                all_zero &= row[1 + t] == 0;
            }
            for (auto tp : places)
                p0 += m0[tp];
        }

        if (all_le_zero) {
            result[pi].first = p0;
            result[pi].second = all_zero;
            if (pi == places.size()) {
                return result;
            }
            continue;
        }

        // Set objective

        auto tmp_lp = context.make_base_lp();
        if (tmp_lp == nullptr)
            return result;

        // Max the objective
        glp_set_obj_dir(tmp_lp, GLP_MAX);

        for (size_t i = 1; i <= nCol; i++) {
            glp_set_obj_coef(tmp_lp, i, row[i]);
            glp_set_col_kind(tmp_lp, i, GLP_IV);
            glp_set_col_bnds(tmp_lp, i, GLP_LO, 0, infty);
        }

        auto rs = glp_simplex(tmp_lp, &settings);
        if (rs == GLP_ETMLIM) {
            std::cerr << "glpk: timeout" << std::endl;
        } else if (rs == 0) {
            auto status = glp_get_status(tmp_lp);
            if (status == GLP_OPT) {
                glp_iocp isettings;
                glp_init_iocp(&isettings);
                isettings.tm_lim =
                    std::max<int>(((double)timeout * 1000) - (glp_time() - stime), 1);
                isettings.msg_lev = 0;
                isettings.presolve = GLP_OFF;
                auto rs = glp_intopt(tmp_lp, &isettings);
                if (rs == GLP_ETMLIM) {
                    std::cerr << "glpk mip: timeout" << std::endl;
                } else if (rs == 0) {
                    auto status = glp_mip_status(tmp_lp);
                    if (status == GLP_OPT) {
                        auto org = p0 + glp_mip_obj_val(tmp_lp);
                        result[pi].first = std::round(org);
                        result[pi].second = all_zero;
                    } else if (status != GLP_UNBND && status != GLP_FEAS) {
                        result[pi].first = p0;
                        result[pi].second = all_zero;
                    }
                }
            } else if (status == GLP_INFEAS || status == GLP_NOFEAS || status == GLP_UNDEF) {
                result[pi].first = p0;
                result[pi].second = all_zero;
            }
        } else if (rs == GLP_ENOPFS || rs == GLP_ENODFS || rs == GLP_ENOFEAS) {
            result[pi].first = p0;
            result[pi].second = all_zero;
        }
        glp_erase_prob(tmp_lp);
        if (pi == places.size() && result[places.size()].first >= p0) {
            return result;
        }
        if (pi == places.size() && places.size() == 1) {
            result[0] = result[1];
            return result;
        }
    }
    return result;
}

void LinearProgram::make_union(const LinearProgram &other) {
    if (_result == IMPOSSIBLE || other._result == IMPOSSIBLE) {
        _result = IMPOSSIBLE;
        _equations.clear();
        assert(false);
        return;
    }

    auto it1 = _equations.begin();
    auto it2 = other._equations.begin();

    while (it1 != _equations.end() && it2 != other._equations.end()) {
        auto cmp = it1->_row.compare(it2->_row);
        if (cmp < 0) {
            ++it1;
        } else if (cmp > 0) {
            it1 = _equations.insert(it1, *it2);
            ++it2;
            ++it1;
        } else {
            equation_t &n = *it1;
            n._lower = std::max(n._lower, it2->_lower);
            n._upper = std::min(n._upper, it2->_upper);
            /*if(n.upper < n.lower)
            {
                _result = result_t::IMPOSSIBLE;
                _equations.clear();
                return;
            }*/
            ++it1;
            ++it2;
        }
    }

    if (it2 != other._equations.end())
        _equations.insert(_equations.end(), it2, other._equations.end());
}
} // namespace Simplification
} // namespace PetriEngine
