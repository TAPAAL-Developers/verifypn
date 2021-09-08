#ifndef LINEARPROGRAMS_H
#define LINEARPROGRAMS_H
#include "../PQL/Contexts.h"
#include "../PetriNet.h"
#include "LinearProgram.h"

#include <set>

namespace PetriEngine::Simplification {

class AbstractProgramCollection {
  protected:
    enum result_t_e { UNKNOWN, IMPOSSIBLE, POSSIBLE };
    result_t_e _result = result_t_e::UNKNOWN;

    virtual void satisfiable_impl(const PQL::SimplificationContext &context,
                                  uint32_t solvetime) = 0;
    bool _has_empty = false;

  public:
    virtual ~AbstractProgramCollection() = default;
    ;
    auto empty() -> bool { return _has_empty; }

    virtual auto satisfiable(const PQL::SimplificationContext &context,
                             uint32_t solvetime = std::numeric_limits<uint32_t>::max()) -> bool {
        reset();
        if (context.timeout() || _has_empty || solvetime == 0)
            return true;
        if (_result != UNKNOWN) {
            if (_result == IMPOSSIBLE) {
                return _result == POSSIBLE;
            }
        }
        satisfiable_impl(context, solvetime);
        assert(_result != UNKNOWN);
        return _result == POSSIBLE;
    }

    auto known_sat() -> bool { return _result == POSSIBLE; };
    auto known_unsat() -> bool { return _result == IMPOSSIBLE; };

    virtual void clear() = 0;

    virtual auto merge(bool &has_empty, LinearProgram &program, bool dry_run = false) -> bool = 0;
    virtual void reset() = 0;
    [[nodiscard]] virtual auto size() const -> size_t = 0;
};

using AbstractProgramCollection_ptr = std::shared_ptr<AbstractProgramCollection>;

class UnionCollection : public AbstractProgramCollection {
  protected:
    std::vector<AbstractProgramCollection_ptr> _lps;
    size_t _current = 0;
    size_t _size = 0;

    void satisfiable_impl(const PQL::SimplificationContext &context, uint32_t solvetime) override {
        for (int i = _lps.size() - 1; i >= 0; --i) {
            if (_lps[i]->satisfiable(context, solvetime) || context.timeout()) {
                _result = POSSIBLE;
                return;
            } else {
                _lps.erase(_lps.begin() + i);
            }
        }
        if (_result != POSSIBLE)
            _result = IMPOSSIBLE;
    }

  public:
    UnionCollection(std::vector<AbstractProgramCollection_ptr> &&programs)
        : AbstractProgramCollection(), _lps(std::move(programs)) {
        for (auto &p : _lps)
            _size += p->size();
    }

    UnionCollection(const AbstractProgramCollection_ptr &A, const AbstractProgramCollection_ptr &B)
        : AbstractProgramCollection(), _lps({A, B}) {
        _has_empty = false;
        for (auto &lp : _lps) {
            _has_empty = _has_empty || lp->empty();
            if (lp->known_sat() || _has_empty)
                _result = POSSIBLE;
            if (_result == POSSIBLE)
                break;
        }
        for (auto &p : _lps)
            _size += p->size();
    };

    void clear() override {
        _lps.clear();
        _current = 0;
    };

    void reset() override {
        _lps[0]->reset();
        _current = 0;
    }

    auto merge(bool &has_empty, LinearProgram &program, bool dry_run = false) -> bool override {

        if (_current >= _lps.size()) {
            _current = 0;
        }

        if (!_lps[_current]->merge(has_empty, program, dry_run)) {
            ++_current;
            if (_current < _lps.size())
                _lps[_current]->reset();
        }

        return _current < _lps.size();
    }
    [[nodiscard]] auto size() const -> size_t override { return _size; }
};

class MergeCollection : public AbstractProgramCollection {
  protected:
    AbstractProgramCollection_ptr _left = nullptr;
    AbstractProgramCollection_ptr _right = nullptr;

    LinearProgram _tmp_prog;
    bool _merge_right = true;
    bool _more_right = true;
    bool _rempty = false;
    size_t _nsat = 0;
    size_t _curr = 0;
    size_t _size = 0;

    void satisfiable_impl(const PQL::SimplificationContext &context, uint32_t solvetime) override {
        // this is where the magic needs to happen
        bool hasmore = false;
        do {
            if (context.timeout()) {
                _result = POSSIBLE;
                break;
            }
            LinearProgram prog;
            bool has_empty = false;
            hasmore = merge(has_empty, prog);
            if (has_empty) {
                _result = POSSIBLE;
                return;
            } else {
                if (context.timeout() || !prog.is_impossible(context, solvetime)) {
                    _result = POSSIBLE;
                    break;
                }
            }
            ++_nsat;
        } while (hasmore);
        if (_result != POSSIBLE)
            _result = IMPOSSIBLE;
        return;
    }

  public:
    MergeCollection(const AbstractProgramCollection_ptr &A, const AbstractProgramCollection_ptr &B)
        : AbstractProgramCollection(), _left(A), _right(B) {
        assert(A);
        assert(B);
        _has_empty = _left->empty() && _right->empty();
        _size = _left->size() * _right->size();
    };

    void reset() override {
        if (_right)
            _right->reset();

        _merge_right = true;
        _more_right = true;
        _rempty = false;

        _tmp_prog = LinearProgram();
        _curr = 0;
    }

    void clear() override {
        _left = nullptr;
        _right = nullptr;
    };

    auto merge(bool &has_empty, LinearProgram &program, bool dry_run = false) -> bool override {
        if (program.known_impossible())
            return false;
        bool lempty = false;
        bool more_left;
        while (true) {
            lempty = false;
            LinearProgram prog = program;
            if (_merge_right) {
                assert(_more_right);
                _rempty = false;
                _tmp_prog = LinearProgram();
                _more_right = _right->merge(_rempty, _tmp_prog, false);
                _left->reset();
                _merge_right = false;
            }
            ++_curr;
            assert(_curr <= _size);
            more_left = _left->merge(lempty, prog /*, dry_run || curr < nsat*/);
            if (!more_left)
                _merge_right = true;
            if (_curr >= _nsat || !(more_left || _more_right)) {
                if ((!dry_run && prog.known_impossible()) && (more_left || _more_right)) {
                    continue;
                }
                if (!dry_run)
                    program.swap(prog);
                break;
            }
        }
        if (!dry_run)
            program.make_union(_tmp_prog);
        has_empty = lempty && _rempty;
        return more_left || _more_right;
    }

    [[nodiscard]] auto size() const -> size_t override { return _size - _nsat; }
};

class SingleProgram : public AbstractProgramCollection {
  private:
    LinearProgram _program;

  protected:
    void satisfiable_impl(const PQL::SimplificationContext &context, uint32_t solvetime) override {
        // this is where the magic needs to happen
        if (!_program.is_impossible(context, solvetime)) {
            _result = POSSIBLE;
        } else {
            _result = IMPOSSIBLE;
        }
    }

  public:
    SingleProgram() : AbstractProgramCollection() { _has_empty = true; }

    SingleProgram(const Member &lh, int constant, op_e op)
        : AbstractProgramCollection(), _program(lh.variables(), constant, op) {
        _has_empty = _program.size() == 0;
        assert(!_has_empty);
    }

    ~SingleProgram() override = default;

    void reset() override {}

    auto merge(bool &has_empty, LinearProgram &program, bool dry_run = false) -> bool override {
        if (dry_run)
            return false;
        program.make_union(this->_program);
        has_empty = this->_program.equations().size() == 0;
        assert(has_empty == this->_has_empty);
        return false;
    }

    void clear() override {}

    [[nodiscard]] auto size() const -> size_t override { return 1; }
};
} // namespace PetriEngine::Simplification

#endif /* LINEARPROGRAMS_H */
