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
 * \file Buffer.cpp
 *
 * \date 2011-08-21
 * \author Philipp Kegel
 */

#include "Buffer.h"

#include "../Context.h"
#include "../Device.h"
#include "../Memory.h"
#include "../Retainable.h"

#include "Error.h"
#include "utility.h"

#include "detail/MappedMemory.h"

#include <dclasio/message/CreateBuffer.h>
#include <dclasio/message/DeleteMemory.h>

#include <dcl/CLError.h>
#include <dcl/ComputeNode.h>
#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>

#include <dcl/util/Logger.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

namespace dclicd {

Buffer::Buffer(
        cl_context context,
        cl_mem_flags flags,
        size_t size,
        void *host_ptr) :
    _cl_mem(context, flags, size, host_ptr), _associatedMemory(nullptr), _offset(0) {
    try {
        /*
         * Create buffer
         */
        dclasio::message::CreateBuffer request(_id, _context->remoteId(), flags, size);

        /* Tentative solution:
         * Copy data to all compute nodes of context, if CL_MEM_COPY_HOST_PTR or
         * CL_MEM_USE_HOST_PTR (cached copy) is specified.
         * The memory objects on the compute nodes will not be updated by the
         * memory coherency protocol, as there is no synchronization point (an
         * event) before the first command actually uses the memory object on
         * any compute node. */
        if (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR)) {
            const auto& computeNodes = _context->computeNodes();
            std::vector<std::shared_ptr<const dcl::DataTransfer>> dataTransfers;

            /* TODO Never send data to compute nodes when creating a memory object
             * The current solution is working but introduces considerable
             * communication overhead as the data is sent to *all* compute nodes.
             * For an efficient implementation, do not copy this data to any
             * compute node before the memory object is accessed by a device on
             * a compute node. However, if CL_MEM_COPY_HOST_PTR has been
             * specified the data has to be copied to a local buffer (_data). */

            // send request and data
            for (auto computeNode : computeNodes) {
                assert(computeNode != nullptr);

                computeNode->sendRequest(request);
                auto sending =
                        computeNode->sendData(size, host_ptr); // copy host pointer
                dataTransfers.push_back(sending);
            }

            // await responses from all compute nodes
            for (auto computeNode : computeNodes) {
                /* TODO Await completion of data transfer
                 * Handle failed data transfers explicitly, as possibly no
                 * response will be sent in that case. */

                computeNode->awaitResponse(request);
                /* TODO Receive responses from *all* compute nodes, i.e., do not stop receipt on first failure */
            }
        } else {
            dcl::executeCommand(_context->computeNodes(), request);
        }

        dcl::util::Logger << dcl::util::Info
                << "Buffer created (ID=" << _id << ')' << std::endl;
    } catch (const dcl::CLError& err) {
        throw Error(err);
    } catch (const dcl::IOException& err) {
        throw Error(err);
    } catch (const dcl::ProtocolException& err) {
        throw Error(err);
    }
}

Buffer::~Buffer() { }

void * Buffer::map(cl_map_flags flags, size_t offset, size_t cb) {
    void *ptr = nullptr;

    if (flags != CL_MAP_READ &&
            flags != CL_MAP_WRITE &&
            flags != (CL_MAP_READ | CL_MAP_WRITE)) {
        // mapping flags are invalid
        throw Error(CL_INVALID_VALUE);
    }
    if (offset + cb > _size) throw Error(CL_INVALID_VALUE);

    {
        std::lock_guard<std::mutex> lock(_dataMutex);
        allocHostMemory();
        ptr = static_cast<unsigned char *>(_data) + offset; // derive ptr from cache or host_ptr
        _mappedRegions.insert(std::make_pair(
                ptr, detail::MappedBufferRegion(flags, offset, cb)));
    }

    return ptr;
}

void Buffer::unmap(void *mappedPtr) {
    std::lock_guard<std::mutex> lock(_dataMutex);
    if (_mappedRegions.erase(mappedPtr) != 1) {
        /* mappedPtr does not point to a mapped region of this memory object */
        throw Error(CL_INVALID_VALUE);
    }
    /* TODO Release host memory */
//    freeHostMemory();
}

const detail::MappedBufferRegion * Buffer::findMapping(void *mappedPtr) const {
    std::lock_guard<std::mutex> lock(_dataMutex);

    auto i = _mappedRegions.find(mappedPtr);
    if (i != std::end(_mappedRegions)) {
        return &i->second;
    } else { // mapping not found
        return nullptr;
    }
}

cl_mem_object_type Buffer::type() const {
    return CL_MEM_OBJECT_BUFFER;
}

cl_uint Buffer::mapCount() const {
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _mappedRegions.size();
}

cl_mem Buffer::associatedMemObject() const {
    return nullptr;
}

size_t Buffer::offset() const {
    return 0;
}

} /* namespace dclicd */
