#ifndef LINEARPROGRAM_H
#define LINEARPROGRAM_H

#include "../PetriNet.h"
#include "Member.h"
#include "Vector.h"
#include "../PQL/Contexts.h"

#include <algorithm>
#include <unordered_set>
#include <memory>
#include <glpk.h>

namespace PetriEngine {
    namespace Simplification {

        struct equation_t
        {
            double _upper = std::numeric_limits<double>::infinity();
            double _lower = -std::numeric_limits<double>::infinity();
            double operator [] (size_t i) const
            {
                return i > 0 ? _upper : _lower;
            }
            Vector _row;

            equation_t(const std::vector<int>& data)
            : _row(data) {}
        };

        class LinearProgram {
        private:
            enum result_t { UKNOWN, IMPOSSIBLE, POSSIBLE };
            result_t _result = result_t::UKNOWN;
            std::vector<equation_t> _equations;
        public:
            void swap(LinearProgram& other)
            {
                std::swap(_result, other._result);
                std::swap(_equations, other._equations);
            }
            virtual ~LinearProgram();
            LinearProgram()
            {
            };

            LinearProgram(const LinearProgram& other)
            : _result(other._result), _equations(other._equations)
            {

            }

            LinearProgram(const std::vector<int>& vec, int constant, op_t op);
            size_t size() const
            {
                return _equations.size();
            }

            const std::vector<equation_t>& equations() const
            {
                return _equations;
            }

            bool known_impossible() const { return _result == result_t::IMPOSSIBLE; }
            bool known_possible() const { return _result == result_t::POSSIBLE; }
            bool is_impossible(const PQL::SimplificationContext& context, uint32_t solvetime);

            void make_union(const LinearProgram& other);

            std::ostream& print(std::ostream& ss, size_t indent = 0) const
            {
                for(size_t i = 0; i < indent ; ++i) ss << "\t";
                ss << "### LP\n";

                for(const equation_t& eq : _equations)
                {
                    for(size_t i = 0; i < indent ; ++i) ss << "\t";
                    eq._row.print(ss);
                    ss << " IN [" << eq._lower << ", " << eq._upper << "]\n";
                }

                for(size_t i = 0; i < indent ; ++i) ss << "\t";
                ss << "### LP DONE";
                return ss;
            }
            static std::vector<std::pair<double,bool>> bounds(const PQL::SimplificationContext& context, uint32_t solvetime, const std::vector<uint32_t>& places);
        };
    }
}

#endif /* LINEARPROGRAM_H */

