/*
 * File:   LPFactory.h
 * Author: Peter G. Jensen
 *
 * Created on 31 May 2017, 09:26
 */


#include <vector>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

#ifndef VECTOR_H
#define VECTOR_H

namespace PetriEngine {
    namespace Simplification {
        enum op_t
        {
            OP_EQ,
            OP_LE,
            OP_GE,
            OP_LT,
            OP_GT,
            OP_NE
        };
        class Vector {
        public:

            bool operator ==(const Vector& other) const
            {
                return  _data == other._data;
            }

            int compare(const Vector& other) {
                if(_data.size() != other._data.size())
                    return (_data.size() < other._data.size()) ? -1 : 1;
                for(size_t i = 0; i < _data.size(); ++i)
                {
                    if(_data[i].first != other._data[i].first)
                        return (_data[i].first < other._data[i].first) ? -1 : 1;
                    if(_data[i].second != other._data[i].second)
                        return (_data[i].second < other._data[i].second) ? -1 : 1;
                }
                return 0;
            }

            const void* raw() const
            {
                return _data.data();
            }

            std::ostream& print(std::ostream& ss) const
            {
                int index = 0;
                for(const std::pair<int,int>& el : _data)
                {
                    while(index < el.first) { ss << "0 "; ++index; }
                    ss << el.second << " ";
                    ++index;
                }
                return ss;
            }

            void write(std::vector<double>& dest) const
            {
                memset(dest.data(), 0, sizeof (double) * dest.size());

                for(const std::pair<int,int>& el : _data)
                {
                    dest[el.first + 1] = el.second;
                }
            }

            size_t write_indir(std::vector<double>& dest, std::vector<int32_t>& indir) const
            {
                size_t l = 1;
                for(const std::pair<int,int>& el : _data)
                {
                    dest[l] = el.second;
                    if(dest[l] != 0)
                    {
                        indir[l] = el.first + 1;
                        ++l;
                    }
                }
                return l;
            }

            Vector(const std::vector<int>& data)
            {
                for(size_t i = 0; i < data.size(); ++i)
                {
                    if(data[i] != 0)
                    {
                        _data.emplace_back(i, data[i]);
                    }
                }
            }

        private:

            std::vector<std::pair<int,int>> _data;
        };
    }
}

#endif /* VECTOR_H */

