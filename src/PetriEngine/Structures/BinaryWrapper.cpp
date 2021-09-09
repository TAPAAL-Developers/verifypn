/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2016  Peter Gjøl Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   binarywrapper.cpp
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 19:20
 */

#include "PetriEngine/Structures/BinaryWrapper.h"
namespace ptrie
{
    
    const uchar BinaryWrapper::_masks[8] = {
        static_cast <uchar>(0x80),
        static_cast <uchar>(0x40),
        static_cast <uchar>(0x20),
        static_cast <uchar>(0x10),
        static_cast <uchar>(0x08),
        static_cast <uchar>(0x04),
        static_cast <uchar>(0x02),
        static_cast <uchar>(0x01)
    };
            
    size_t BinaryWrapper::overhead(uint size)
    {
        size = size % 8;
        if (size == 0)
            return 0;
        else
            return 8 - size; 
    }
    
    
    size_t BinaryWrapper::bytes(uint size)
    {
        return (size + overhead(size))/8;
    }
    
    
    BinaryWrapper::BinaryWrapper(uint size)
    {
        _nbytes = (size + overhead(size)) / 8;
        _blob = zallocate(_nbytes);
    }
    
    
    BinaryWrapper::BinaryWrapper(const BinaryWrapper& other, uint offset)
    {
         offset = offset / 8;

        _nbytes = other._nbytes/*.load()*/;
        if (_nbytes > offset)
            _nbytes -= offset;
        else {
            _nbytes = 0;
        }

        _blob = allocate(_nbytes);
        memcpy(raw(), &(other.const_raw()[offset]), _nbytes);
        assert(raw()[0] == other.const_raw()[offset]);
    }
    
    BinaryWrapper::BinaryWrapper
        (uchar* org, uint size, uint offset, uint encodingsize)
    {
        if(size == 0 || offset >= encodingsize)
        {
            _nbytes = 0;
            _blob = nullptr;            
            return;
        }
        
        uint so = size + offset;
        offset = ((so - 1) / 8) - ((size - 1) / 8);

        _nbytes = ((encodingsize + this->overhead(encodingsize)) / 8);
        if (_nbytes > offset)
            _nbytes -= offset;
        else {
            _nbytes = 0;
            _blob = nullptr;
            return;
        }

        uchar* tmp = &(org[offset]);
        if(_nbytes <= BW_BSIZE)
        {
            memcpy(raw(), tmp, _nbytes);            
        }
        else
        {
            _blob = tmp;
        }
        assert(org[offset] == raw()[0]);

    }
    
    
    BinaryWrapper::BinaryWrapper(uchar* org, uint size)
    {
        _nbytes = size / 8 + (size % 8 ? 1 : 0);     
        _blob = org;
        
        if(_nbytes <= BW_BSIZE)
            memcpy(raw(), org, _nbytes);
        
//        assert(raw[0] == const_raw()[0]);
    }
    
    
    void BinaryWrapper::copy(const BinaryWrapper& other, uint offset)
    {
        memcpy(&(raw()[offset / 8]), other.const_raw(), other._nbytes);
        assert(other.const_raw()[0] == raw()[0]);
    }
    
    
    void BinaryWrapper::copy(const uchar* data, uint size)
    {
        if(size > 0)
        {
            _blob = allocate(size);
            _nbytes = size;
            memcpy(raw(), data, size);
            assert(data[0] == raw()[0]);
        }
        else
        {
            _nbytes = 0;
            release();
        }
    }
        
    // accessors
    
    void BinaryWrapper::print(std::ostream& stream, size_t length) const
    {
        stream << _nbytes << " bytes : ";
        for (size_t i = 0; i < _nbytes * 8 && i < length; i++) {
            if (i % 8 == 0 && i != 0) stream << "-";
            stream << this->at(i);
        }
    }
}

namespace std {
    std::ostream &operator<<(std::ostream &os, const ptrie::BinaryWrapper &b) {
        b.print(os);
        return os;
    }
}
