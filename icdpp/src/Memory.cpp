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

/**
 * @file Memory.cpp
 *
 * @date 2011-08-21
 * @author Philipp Kegel
 */

#if defined(linux) || defined(__linux) || defined(__linux__)
/* Feature test macro to enable posix_memalign in stdlib.h */
#define _POSIX_C_SOURCE 200112L
#endif

#include "Memory.h"

#include "Context.h"
#include "Device.h"
#include "Retainable.h"

#include "dclicd/Error.h"
#include "dclicd/utility.h"

#include "dclicd/detail/MappedMemory.h"

#include <dclasio/message/CreateBuffer.h>
#include <dclasio/message/DeleteMemory.h>

#include <dcl/CLError.h>
#include <dcl/ComputeNode.h>
#include <dcl/DataTransfer.h>
#include <dcl/DCLException.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

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

#ifdef DCL_MEM_LOCK
#if defined(linux) || defined(__linux) || defined(__linux__)
#include <cerrno>
#include <sys/mman.h>
#include <sys/resource.h>
#endif
#endif
#if defined(linux) || defined(__linux) || defined(__linux__)
#include <unistd.h>
#endif


std::set<cl_mem> _cl_mem::_created_mem_obj;

cl_mem _cl_mem::findMemObject(cl_mem ptr) {
	auto i = std::find(std::begin(_created_mem_obj), std::end(_created_mem_obj), ptr);
	return ((i == std::end(_created_mem_obj)) ? nullptr : ptr);
}

_cl_mem::_cl_mem(
		cl_context context,
		cl_mem_flags flags,
		size_t size,
		void *host_ptr) :
	_context(context), _flags(flags), _size(size), _host_ptr(host_ptr), _data(nullptr) {
	/* Read-write mode of memory object */
    cl_mem_flags rwMode = flags &
    		(CL_MEM_READ_WRITE | CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY);
    /* Allocate memory from host-accessible memory
     * Host-accessible memory (e.g., PCIe memory) is part of the host memory
     * that physically is accessible by both, host *and* device.
	 * In dOpenCL, this kind of memory could only be provided by highly
	 * sophisticated memory controllers that provide a uniform physical memory
	 * space across a network networks.
	 * However, in dOpenCL we take this flag as a hint to use page-locked host
	 * memory. */
    bool allocHostPtr = (flags & CL_MEM_ALLOC_HOST_PTR);
    /* Copy or use memory referenced by host pointer */
    cl_mem_flags hostPtrMode = flags &
    		(CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR);

    /* Validate context */
	if (!context) throw dclicd::Error(CL_INVALID_CONTEXT);

	/* TODO Assert 0 < size <= CL_DEVICE_MAX_MEM_ALLOC_SIZE */
	if (size == 0) throw dclicd::Error(CL_INVALID_BUFFER_SIZE);

	/*
	 * Validate flags
	 */
    /* Read-write flags are mutually exclusive;
     * if no rwMode is given CL_MEM_READ_WRITE is the default */
	if (rwMode &&
			rwMode != CL_MEM_READ_WRITE &&
    		rwMode != CL_MEM_READ_ONLY &&
    		rwMode != CL_MEM_WRITE_ONLY) {
        throw dclicd::Error(CL_INVALID_VALUE);
    }

    /* Host pointer flags */
    if (host_ptr) {
    	switch (hostPtrMode) {
    	case CL_MEM_COPY_HOST_PTR:
    	    /* allocate memory for copying the host pointer */
    	    allocHostMemory();
    	    /* TODO Copy data at host pointer to memory object's host memory */
    		break;
    	case CL_MEM_USE_HOST_PTR:
    		/* CL_MEM_USE_HOST_PTR and CL_MEM_ALLOC_HOST_PTR are mutually
    		 * exclusive */
    		if (allocHostPtr) throw dclicd::Error(CL_INVALID_VALUE);

    		/* Use memory referenced by host pointer as the storage bits for
    		 * the memory object.
    		 * Note that the OpenCL implementation is allowed to cache the
    		 * memory object contents in device memory */
    		_data = _host_ptr;

            /* TODO Update host data at synchronization points, if
             *      CL_MEM_USE_HOST_PTR is specified
             * Copy data from compute node when clWaitForEvents or clFinish is
             * called and affects an event or a command-queue that modified this
             * memory object */
    		break;
    	case (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR):
			/* CL_MEM_COPY_HOST_PTR and CL_MEM_USE_HOST_PTR are mutually
			 * exclusive */
			throw dclicd::Error(CL_INVALID_VALUE);
    	default:
    		/* Any of CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR must be set,
    		 * if host pointer is not NULL */
    		throw dclicd::Error(CL_INVALID_HOST_PTR);
    	}
    } else {
    	/* CL_MEM_COPY_HOST_PTR and CL_MEM_USE_HOST_PTR are only valid, if host
    	 * pointer is not NULL */
    	if (hostPtrMode) throw dclicd::Error(CL_INVALID_HOST_PTR);
    }

	_context->retain();

	/* Add memory object instance to list of memory object instances */
    bool inserted = _cl_mem::_created_mem_obj.insert(this).second;
    assert(inserted);
}

_cl_mem::~_cl_mem() {
    freeHostMemory();

    dclicd::release(_context);

	/* remove buffer from list of valid memory objects */
	_cl_mem::_created_mem_obj.erase(this);
}

void _cl_mem::allocHostMemory() {
    if (_data) return; // host pointer already allocated

#if defined(linux) || defined(__linux) || defined(__linux__)
    /* align memory to page boundaries in order to minimize the amount
     * of required pages */
    int err = ::posix_memalign(&_data, ::sysconf(_SC_PAGESIZE), _size);
    if (err != 0) throw dclicd::Error(CL_MEM_OBJECT_ALLOCATION_FAILURE);
#else  /* Linux */
    _data = (void *) ::malloc(_size);
    if (_data == nullptr) throw dclicd::Error(CL_MEM_OBJECT_ALLOCATION_FAILURE);
#endif /* Linux */

    /* CL_MEM_ALLOC_HOST_PTR has been requested, page-lock the host memory
     * for storing the memory object */
    if ((_flags & CL_MEM_ALLOC_HOST_PTR)) lockHostMemory();
}

void _cl_mem::freeHostMemory() {
    if (!_data) return; // no host pointer allocated

    unlockHostMemory();

    if (_data != _host_ptr) {
        /* delete host memory data */
        ::free(_data);
    }
}

void _cl_mem::lockHostMemory() {
#ifdef DCL_MEM_LOCK
    assert(_data != nullptr);

#ifdef _POSIX_MEMLOCK_RANGE
#if defined(linux) || defined(__linux) || defined(__linux__)
    struct rlimit mlock_limit;

    ::getrlimit(RLIMIT_MEMLOCK, &mlock_limit);
    /* Linux automatically rounds addr (i.e., _data) to page boundaries.
     * However, as this is not required by POSIX, a portable implementation
     * must not rely on proper alignment of addr. */
    if (::mlock(_data, _size) != 0) {
        /* memory locking failed */
        std::cerr << "Cannot lock host memory for memory object: " << strerror(errno)
                << " (mlock limit cur=" << mlock_limit.rlim_cur
                << " bytes, max=" << mlock_limit.rlim_max << " bytes)" << std::endl;
    }
#endif /* Linux */
#endif /* _POSIX_MEMLOCK_RANGE */
#endif /* DCL_MEM_LOCK */
}

void _cl_mem::unlockHostMemory() {
    if (!(_flags & CL_MEM_ALLOC_HOST_PTR)) return; // host memory has not been locked

#ifdef DCL_MEM_LOCK
    assert(_data != nullptr);

#ifdef _POSIX_MEMLOCK_RANGE
#if defined(linux) || defined(__linux) || defined(__linux__)
    /* Linux automatically rounds addr (i.e., _data) to page boundaries.
     * However, as this is not required by POSIX, a portable implementation
     * must not rely on proper alignment of addr. */
    if (::munlock(_data, _size) != 0) {
        /* memory unlocking failed */
        std::cerr << "Could not unlock memory object in host memory: "
                << strerror(errno) << std::endl;
    }
#endif /* Linux */
#endif /* _POSIX_MEMLOCK_RANGE */
#endif /* DCL_MEM_LOCK */
}

void _cl_mem::destroy() {
	assert(_ref_count == 0);

	/* execute destructor callbacks
	 * Callbacks must be executed *before* the memory object's resources are freed. */
	for (auto callback : _destructorCallbacks) {
	    callback.first(this, callback.second);
	}

	try {
		dclasio::message::DeleteMemory request(_id);
		dcl::executeCommand(_context->computeNodes(), request);
		dcl::util::Logger << dcl::util::Info
				<< "Memory object deleted (ID=" << _id << ')' << std::endl;
	} catch (const dcl::CLError& err) {
		throw dclicd::Error(err);
	} catch (const dcl::IOException& err) {
		throw dclicd::Error(err);
	} catch (const dcl::ProtocolException& err) {
		throw dclicd::Error(err);
	}
}

cl_context _cl_mem::context() const {
	return _context;
}

void _cl_mem::getInfo(
        cl_mem_info param_name,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret) const {
    switch (param_name) {
    case CL_MEM_TYPE:
        dclicd::copy_info(type(), param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_FLAGS:
        dclicd::copy_info(_flags, param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_SIZE:
        dclicd::copy_info(_size, param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_HOST_PTR:
        dclicd::copy_info(_host_ptr, param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_MAP_COUNT:
        dclicd::copy_info(mapCount(), param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_REFERENCE_COUNT:
        dclicd::copy_info(_ref_count, param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_CONTEXT:
        dclicd::copy_info(_context, param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_ASSOCIATED_MEMOBJECT:
        dclicd::copy_info(associatedMemObject(), param_value_size, param_value,
                param_value_size_ret);
        break;
    case CL_MEM_OFFSET:
        dclicd::copy_info(offset(), param_value_size, param_value,
                param_value_size_ret);
        break;
//    case CL_MEM_D3D10_RESOURCE_KHR:
//        assert(!"cl_khr_d3d10_sharing not supported by dOpenCL");
//        break;
    default:
        throw dclicd::Error(CL_INVALID_VALUE);
    }
}

void _cl_mem::setDestructorCallback(
		void (CL_CALLBACK *pfn_notify)(cl_mem, void *), void *user_data) {
	_destructorCallbacks.push_back(std::make_pair(pfn_notify, user_data));
}

bool _cl_mem::isOutput() const {
    return (_flags & (CL_MEM_WRITE_ONLY | CL_MEM_READ_WRITE));
}

void _cl_mem::onAcquireComplete(dcl::Process& destination, cl_int executionStatus) {
    assert(executionStatus == CL_COMPLETE || executionStatus < 0);

    if (executionStatus == CL_COMPLETE) {
        /* forward acquired memory object data to acquiring compute node */
        try {
            destination.sendData(_size, _data);
        } catch (const dcl::IOException& e) {
            dcl::util::Logger << dcl::util::Error
                    << "(SYN) Acquire failed: " << e.what()
                    << std::endl;
        }
    } else {
        dcl::util::Logger << dcl::util::Error
                << "(SYN) Acquire failed: Data receipt failed"
                << std::endl;
    }
}

void _cl_mem::onAcquire(dcl::Process& destination, dcl::Process& source) {
    dcl::util::Logger << dcl::util::Debug
            << "(SYN) Acquiring memory object from compute node '" << source.url()
            << "' on behalf of compute node '" << destination.url()
            << " (ID=" << remoteId() << ')'
            << std::endl;

    /* acquire data from source compute node */
    /* FIXME Do not use memory object's host data cache for synchronizing compute node events
     * A host maintains its own copy of a memory object (when allocated with
     * CL_MEM_ALLOC_HOST_PTR) which must not be overwritten by possibly
     * different copies that are exchanged during synchronization between
     * compute nodes */
    try {
        auto recv = acquire(source);

        /* forward memory object data to requesting compute node */
        recv->setCallback(
                std::bind(&_cl_mem::onAcquireComplete, this,
                        std::ref(destination), std::placeholders::_1));
    } catch (const dcl::IOException& e) {
        dcl::util::Logger << dcl::util::Error
                << "(SYN) Acquire failed: " << e.what()
                << std::endl;
    }
}

std::shared_ptr<dcl::DataTransfer> _cl_mem::acquire(dcl::Process& process) {
    std::lock_guard<std::mutex> lock(_dataMutex);
    allocHostMemory();
    return process.receiveData(_size, _data);
}
