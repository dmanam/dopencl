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
 * \file Memory.cpp
 *
 * \date 2011-09-01
 * \author Philipp Kegel
 */

#include "Memory.h"

#include "Context.h"

#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/Process.h>

#include <dcl/util/Logger.h>

#define __CL_ENABLE_EXCEPTIONS
#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.hpp>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>

namespace {

struct ExecData {
    dcl::Process *process;
    size_t size;
    void *ptr;
    cl::UserEvent event;
};

void execAcquire(cl_event event, cl_int execution_status, void *user_data) {
    std::unique_ptr<ExecData> syncData(static_cast<ExecData *>(user_data));

    assert(execution_status == CL_COMPLETE || execution_status < 0);
    assert(syncData != nullptr);

    if (execution_status == CL_COMPLETE) {
        dcl::util::Logger << dcl::util::Debug
                << "(SYN) Acquiring memory object data from process '"
                << syncData->process->url() << '\''
                << std::endl;

        try {
            auto recv = syncData->process->receiveData(syncData->size, syncData->ptr);
            recv->setCallback(
                    std::bind(&cl::UserEvent::setStatus, syncData->event, std::placeholders::_1));
        } catch (const dcl::IOException& e) {
            dcl::util::Logger << dcl::util::Error
                    << "Data receipt failed: " << e.what() << std::endl;
            syncData->event.setStatus(CL_IO_ERROR_WWU);
        }
    } else {
        dcl::util::Logger << dcl::util::Error
                << "(SYN) Acquiring memory object data failed"
                << std::endl;

        syncData->event.setStatus(execution_status);
    }
}

void execRelease(cl_event event, cl_int execution_status, void *user_data) {
    std::unique_ptr<ExecData> syncData(static_cast<ExecData *>(user_data));

    assert(execution_status == CL_COMPLETE || execution_status < 0);
    assert(syncData != nullptr);

    if (execution_status == CL_COMPLETE) {
        dcl::util::Logger << dcl::util::Debug
                << "(SYN) Releasing memory object data to process '"
                << syncData->process->url() << '\''
                << std::endl;

        try {
            auto send = syncData->process->sendData(syncData->size, syncData->ptr);
            send->setCallback(
                    std::bind(&cl::UserEvent::setStatus, syncData->event, std::placeholders::_1));
        } catch (const dcl::IOException& e) {
            dcl::util::Logger << dcl::util::Error
                    << "Data sending failed: " << e.what() << std::endl;
            syncData->event.setStatus(CL_IO_ERROR_WWU);
        }
    } else {
        dcl::util::Logger << dcl::util::Error
                << "(SYN) Releasing memory object data failed"
                << std::endl;

        syncData->event.setStatus(execution_status);
    }
}

} /* unnamed namespace */

/* ****************************************************************************/

namespace dcld {

/* ****************************************************************************
 * Memory object
 ******************************************************************************/

Memory::Memory(const std::shared_ptr<Context>& context) :
        _context(context) {
}

Memory::~Memory() { }

size_t Memory::size() const {
    return static_cast<cl::Memory>(*this).getInfo<CL_MEM_SIZE>();
}

bool Memory::isInput() const{
	return (static_cast<cl::Memory>(*this).getInfo<CL_MEM_FLAGS>() &
	        (CL_MEM_READ_ONLY | CL_MEM_READ_WRITE));
}

bool Memory::isOutput() const {
	return (static_cast<cl::Memory>(*this).getInfo<CL_MEM_FLAGS>() &
	        (CL_MEM_WRITE_ONLY | CL_MEM_READ_WRITE));
}

/* ****************************************************************************
 * Buffer
 ******************************************************************************/

Buffer::Buffer(
        const std::shared_ptr<Context>& context, cl_mem_flags flags,
		size_t size, void *ptr) :
	dcld::Memory(context)
{
    cl_mem_flags rwFlags = flags &
            (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY);
//    cl_mem_flags hostPtrFlags = flags &
//            (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR);
    cl_mem_flags allocHostPtr = flags &
            CL_MEM_ALLOC_HOST_PTR;

    /*
     * Always let the OpenCL implementation allocate the memory on the compute
     * node to ensure optimal performance.
     * If CL_MEM_USE_HOST_PTR is specified, the compute node should try to use
     * pinned memory to ensure optimal performance for frequent data transfers.
     */

    if (ptr) {
        /* TODO Improve data transfer on buffer creation
         * Use map/unmap to avoid explicit memory allocation *or*
         * copy data only on host */
        _buffer = cl::Buffer(*_context, rwFlags | CL_MEM_COPY_HOST_PTR | allocHostPtr, size, ptr);
    } else {
        /* create uninitialized buffer */
        _buffer = cl::Buffer(*_context, rwFlags | allocHostPtr, size);
    }
}

Buffer::~Buffer() { }

Buffer::operator cl::Memory() const {
    return _buffer;
}

Buffer::operator cl::Buffer() const {
	return _buffer;
}

void Buffer::acquire(
        dcl::Process& process,
        const cl::CommandQueue& commandQueue,
        const cl::Event& releaseEvent,
        cl::Event *acquireEvent) {
    cl::Event mapEvent;
    cl::UserEvent dataReceipt(*_context);

    dcl::util::Logger << dcl::util::Debug
            << "(SYN) Acquiring buffer from process '" << process.url() << '\''
            << std::endl;

    /* map buffer to host memory when releaseEvent is complete */
    VECTOR_CLASS<cl::Event> mapWaitList(1, releaseEvent);
    void *ptr = commandQueue.enqueueMapBuffer(
            _buffer,
            CL_FALSE,
            CL_MAP_WRITE,
            0, size(),
            &mapWaitList, &mapEvent);

    auto syncData = new ExecData;
    syncData->process = &process;
    syncData->size    = size();
    syncData->ptr     = ptr;
    syncData->event   = dataReceipt;

    /* receive buffer data when mapping is complete */
    mapEvent.setCallback(CL_COMPLETE, &execAcquire, syncData);

    /* WARNING: do not use syncData after this point, as the callback of
     *          mapEvent deletes it concurrently */

    /* unmap buffer when acquire operation is complete */
    VECTOR_CLASS<cl::Event> unmapWaitList(1, dataReceipt);
    commandQueue.enqueueUnmapMemObject(
            _buffer,
            ptr,
            &unmapWaitList, acquireEvent);
}

void Buffer::release(
        dcl::Process& process,
        const cl::CommandQueue& commandQueue,
        const cl::Event& releaseEvent) const {
    cl::Event mapEvent;
    cl::UserEvent dataSending(*_context);

    dcl::util::Logger << dcl::util::Debug
            << "(SYN) Releasing buffer to process '" << process.url() << '\''
            << std::endl;

    /* map buffer when releaseEvent is complete */
    VECTOR_CLASS<cl::Event> mapWaitList(1, releaseEvent);
    void *ptr = commandQueue.enqueueMapBuffer(
            _buffer,
            CL_FALSE,
            CL_MAP_READ,
            0, size(),
            &mapWaitList, &mapEvent
    );

    auto syncData = new ExecData;
    syncData->process = &process;
    syncData->size    = size();
    syncData->ptr     = ptr;
    syncData->event   = dataSending;

    /* send buffer data when mapping is complete */
    mapEvent.setCallback(CL_COMPLETE, &execRelease, syncData);

    /* WARNING: do not use syncData after this point, as the callback of
     *          mapEvent deletes it concurrently */

    /* unmap buffer when acquire operation is complete */
    VECTOR_CLASS<cl::Event> unmapWaitList(1, dataSending);
    commandQueue.enqueueUnmapMemObject(
            _buffer,
            ptr,
            &unmapWaitList, nullptr);
}

} /* namespace dcld */
