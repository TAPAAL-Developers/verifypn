#ifndef MEMBER_H
#define MEMBER_H
#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <functional>

namespace PetriEngine::Simplification {
enum trivial_e { FALSE = 0, TRUE = 1, INDETERMINATE = 2 };
enum member_type_e { CONSTANT, INPUT, OUTPUT, REGULAR };
class Member {
    std::vector<int> _variables;
    int _constant = 0;
    bool _canAnalyze = false;

  public:
    Member(std::vector<int> &&vec, int constant, bool canAnalyze = true)
        : _variables(vec), _constant(constant), _canAnalyze(canAnalyze) {}
    Member(int constant, bool canAnalyse = true) : _constant(constant), _canAnalyze(canAnalyse) {}
    Member() = default;

    virtual ~Member() = default;

    [[nodiscard]] auto constant() const -> int { return _constant; };
    [[nodiscard]] auto can_analyze() const -> bool { return _canAnalyze; };
    [[nodiscard]] auto size() const -> size_t { return _variables.size(); }
    auto variables() -> std::vector<int> & { return _variables; }
    [[nodiscard]] auto variables() const -> const std::vector<int> & { return _variables; }

    auto operator+=(const Member &m) -> Member & {
        auto tc = _constant + m._constant;
        auto ca = _canAnalyze && m._canAnalyze;
        add_variables(m);
        _constant = tc;
        _canAnalyze = ca;
        return *this;
    }

    auto operator-=(const Member &m) -> Member & {
        auto tc = _constant - m._constant;
        auto ca = _canAnalyze && m._canAnalyze;
        subtract_variables(m);
        _constant = tc;
        _canAnalyze = ca;
        return *this;
    }

    auto operator*=(const Member &m) -> Member & {
        if (!is_zero() && !m.is_zero()) {
            _canAnalyze = false;
            _constant = 0;
            _variables.clear();
        } else {
            auto tc = _constant * m._constant;
            auto ca = _canAnalyze && m._canAnalyze;
            multiply(m);
            _constant = tc;
            _canAnalyze = ca;
        }
        return *this;
    }

    [[nodiscard]] auto substration_is_zero(const Member &m2) const -> bool {
        uint32_t min = std::min(_variables.size(), m2._variables.size());
        uint32_t i = 0;
        for (; i < min; i++) {
            if (_variables[i] != m2._variables[i])
                return false;
        }

        for (; i < _variables.size(); ++i) {
            if (_variables[i] != 0)
                return false;
        }

        for (; i < m2._variables.size(); ++i) {
            if (m2._variables[i] != 0)
                return false;
        }
        return true;
    }

    [[nodiscard]] auto is_zero() const -> bool {
        for (const int &v : _variables) {
            if (v != 0)
                return false;
        }
        return true;
    }

    [[nodiscard]] member_type_e get_type() const {
        bool isConstant = true;
        bool isInput = true;
        bool isOutput = true;
        for (const int &v : _variables) {
            if (v < 0) {
                isConstant = false;
                isOutput = false;
            } else if (v > 0) {
                isConstant = false;
                isInput = false;
            }
        }
        if (isConstant)
            return member_type_e::CONSTANT;
        else if (isInput)
            return member_type_e::INPUT;
        else if (isOutput)
            return member_type_e::OUTPUT;
        else
            return member_type_e::REGULAR;
    }

    auto operator==(const Member &m) const -> bool {
        size_t min = std::min(_variables.size(), m.size());
        size_t max = std::max(_variables.size(), m.size());
        if (memcmp(_variables.data(), m._variables.data(), sizeof(int) * min) != 0) {
            return false;
        }
        for (uint32_t i = min; i < max; i++) {
            if (i >= _variables.size()) {
                if (m._variables[i] != 0)
                    return false;
            } else if (i >= m._variables.size()) {
                if (_variables[i] != 0)
                    return false;
            } else {
                assert(false);
                if (_variables[i] - m._variables[i] != 0)
                    return false;
            }
        }
        return true;
    }

    trivial_e operator<(const Member &m) const { return trivial_less_than(m, std::less<int>()); }
    trivial_e operator<=(const Member &m) const {
        return trivial_less_than(m, std::less_equal<int>());
    }
    trivial_e operator>(const Member &m) const {
        return m.trivial_less_than(*this, std::less<int>());
    }
    trivial_e operator>=(const Member &m) const {
        return m.trivial_less_than(*this, std::less_equal<int>());
    }

  private:
    void add_variables(const Member &m2) {
        uint32_t size = std::max(_variables.size(), m2._variables.size());
        _variables.resize(size, 0);

        for (uint32_t i = 0; i < size; i++) {
            if (i >= m2._variables.size()) {
                break;
            } else {
                _variables[i] += m2._variables[i];
            }
        }
    }

    void subtract_variables(const Member &m2) {
        uint32_t size = std::max(_variables.size(), m2._variables.size());
        _variables.resize(size, 0);

        for (uint32_t i = 0; i < size; i++) {
            if (i >= m2._variables.size()) {
                break;
            } else {
                _variables[i] -= m2._variables[i];
            }
        }
    }

    void multiply(const Member &m2) {

        if (is_zero() != m2.is_zero()) {
            if (!is_zero()) {
                for (auto &v : _variables)
                    v *= m2._constant;
                return;
            } else if (!m2.is_zero()) {
                _variables = m2._variables;
                for (auto &v : _variables)
                    v *= _constant;
                return;
            }
        }
        _variables.clear();
    }

    trivial_e trivial_less_than(const Member &m2,
                                const std::function<bool(int, int)> &compare) const {
        member_type_e type1 = get_type();
        member_type_e type2 = m2.get_type();

        // self comparison
        if (*this == m2)
            return compare(_constant, m2._constant) ? trivial_e::TRUE : trivial_e::FALSE;

        // constant < constant/input/output
        if (type1 == member_type_e::CONSTANT) {
            if (type2 == member_type_e::CONSTANT) {
                return compare(_constant, m2._constant) ? trivial_e::TRUE : trivial_e::FALSE;
            } else if (type2 == member_type_e::INPUT && !compare(_constant, m2._constant)) {
                return trivial_e::FALSE;
            } else if (type2 == member_type_e::OUTPUT && compare(_constant, m2._constant)) {
                return trivial_e::TRUE;
            }
        }
        // input < output/constant
        else if (type1 == member_type_e::INPUT &&
                 (type2 == member_type_e::CONSTANT || type2 == member_type_e::OUTPUT) &&
                 compare(_constant, m2._constant)) {
            return trivial_e::TRUE;
        }
        // output < input/constant
        else if (type1 == member_type_e::OUTPUT &&
                 (type2 == member_type_e::CONSTANT || type2 == member_type_e::INPUT) &&
                 !compare(_constant, m2._constant)) {
            return trivial_e::FALSE;
        }
        return trivial_e::INDETERMINATE;
    }
};
} // namespace PetriEngine::Simplification

#endif /* MEMBER_H */
