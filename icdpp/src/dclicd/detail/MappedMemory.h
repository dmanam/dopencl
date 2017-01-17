/******************************************************************************
 * This file is part of dOpenCL.
 * 
 * dOpenCL is an implementation of the OpenCL application programming
 * interface for distributed systems. See <http://dopencl.uni-muenster.de/>
 * for more information.
 * 
 * Developed by: Research Group Parallel and Distributed Systems
 *               Department of Mathematics and Computer Science
 *               University of Muenster, Germany
 *               <http://pvs.uni-muenster.de/>
 * 
 * Copyright (C) 2013  Philipp Kegel <philipp.kegel@uni-muenster.de>
 *
 * dOpenCL is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * dOpenCL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with dOpenCL. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Permission to use dOpenCL for scientific, non-commercial work is
 * granted under the terms of the dOpenCL Academic License provided
 * appropriate credit is given. See the dOpenCL Academic License for
 * more details.
 * 
 * You should have received a copy of the dOpenCL Academic License
 * along with dOpenCL. If not, see <http://dopencl.uni-muenster.de/>.
 ******************************************************************************/

/*!
 * \file MappedMemory.h
 *
 * \date 2012-08-04
 * \author Philipp Kegel
 */

#ifndef MAPPEDMEMORY_H_
#define MAPPEDMEMORY_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>

namespace dclicd {

namespace detail {

/*!
 * \brief A mapped region of a memory object.
 *
 * This class save information about mapped regions of memory objects, such as
 * the map flags. The map flags are required, to determine whether
 * synchronization of a memory object is required when the mapped region is
 * unmapped.
 */
class MappedMemory {
public:
    MappedMemory(
            cl_map_flags map_flags) :
        _map_flags(map_flags) { }
    virtual ~MappedMemory() { }

    cl_map_flags flags() const {
        return _map_flags;
    }

protected:
    cl_map_flags _map_flags;
};

/******************************************************************************/

/*!
 * \brief A mapped region of a buffer.
 */
class MappedBufferRegion : public MappedMemory {
public:
    MappedBufferRegion(
            cl_map_flags map_flags,
            size_t offset,
            size_t cb) :
        MappedMemory(map_flags), _offset(offset), _cb(cb) { }

    size_t offset() const {
        return _offset;
    }

    size_t cb() const {
        return _cb;
    }

private:
    size_t _offset;
    size_t _cb;
};

} /* namespace detail */

} /* namespace dclicd */

#endif /* MAPPEDMEMORY_H_ */
