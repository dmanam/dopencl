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
 * \file Buffer.h
 *
 * \date 2011-08-21
 * \author Philipp Kegel
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include "../Memory.h"

#include "detail/MappedMemory.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <map>

namespace dclicd {

class Buffer: public _cl_mem {
public:
    Buffer(
            cl_context context,
            cl_mem_flags flags,
            size_t size,
            void *host_ptr);

    virtual ~Buffer();

    /*!
     * \brief Maps a region of this buffer into the host address space and returns a pointer to this mapped region.
     *
     * This method only allocates the pointer to the mapped region of the buffer
     * but does not actually map, i.e. copy its data from a device.
     *
     * \param[in]  flags
     * \param[in]  offset
     * \param[in]  cb
     * \return a pointer to the mapped region of the buffer
     */
    void * map(
            cl_map_flags flags,
            size_t       offset,
            size_t       cb);

    void unmap(
            void *mappedPtr);

    const detail::MappedBufferRegion * findMapping(
            void *mappedPtr) const;

protected:
    cl_mem_object_type type() const ;
    cl_uint mapCount() const;
    cl_mem associatedMemObject() const;
    size_t offset() const;

private:
    /*!
     * \brief A list of mapped regions of this memory object.
     *
     * A pointer for a mapped region is always derived from the data cache of
     * this memory object. The size of this member is the mapCount of this
     * memory object.
     */
    std::map<void *, detail::MappedBufferRegion> _mappedRegions;

    /*
     * Sub-buffer attributes
     */
    cl_mem _associatedMemory;
    size_t _offset;
};

} /* namespace dclicd */

#endif /* BUFFER_H_ */
