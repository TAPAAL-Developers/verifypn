#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H

#include "../PQL/Contexts.h"
#include "../PetriNet.h"
#include "Member.h"
#include "Vector.h"

#include <algorithm>
#include <glpk.h>
#include <memory>
#include <unordered_set>

namespace PetriEngine::Simplification {

struct equation_t {
    double _upper = std::numeric_limits<double>::infinity();
    double _lower = -std::numeric_limits<double>::infinity();
    auto operator[](size_t i) const -> double { return i > 0 ? _upper : _lower; }
    Vector _row;

    equation_t(const std::vector<int> &data) : _row(data) {}
};

class LinearProgram {
  private:
    enum result_e { UKNOWN, IMPOSSIBLE, POSSIBLE };
    result_e _result = result_e::UKNOWN;
    std::vector<equation_t> _equations;

  public:
    void swap(LinearProgram &other) {
        std::swap(_result, other._result);
        std::swap(_equations, other._equations);
    }
    virtual ~LinearProgram();
    LinearProgram() = default;
    ;

    LinearProgram(const LinearProgram &other) = default;

    LinearProgram(const std::vector<int> &vec, int constant, op_e op);
    [[nodiscard]] auto size() const -> size_t { return _equations.size(); }

    [[nodiscard]] auto equations() const -> const std::vector<equation_t> & { return _equations; }

    [[nodiscard]] auto known_impossible() const -> bool {
        return _result == result_e::IMPOSSIBLE;
    }
    [[nodiscard]] auto known_possible() const -> bool { return _result == result_e::POSSIBLE; }
    auto is_impossible(const PQL::SimplificationContext &context, uint32_t solvetime) -> bool;

    void make_union(const LinearProgram &other);

    auto print(std::ostream &ss, size_t indent = 0) const -> std::ostream & {
        for (size_t i = 0; i < indent; ++i)
            ss << "\t";
        ss << "### LP\n";

        for (const equation_t &eq : _equations) {
            for (size_t i = 0; i < indent; ++i)
                ss << "\t";
            eq._row.print(ss);
            ss << " IN [" << eq._lower << ", " << eq._upper << "]\n";
        }

        for (size_t i = 0; i < indent; ++i)
            ss << "\t";
        ss << "### LP DONE";
        return ss;
    }
    static auto bounds(const PQL::SimplificationContext &context, uint32_t solvetime,
                       const std::vector<uint32_t> &places) -> std::vector<std::pair<double, bool>>;
};
} // namespace PetriEngine::Simplification

#endif /* LINEARPROGRAM_H */
