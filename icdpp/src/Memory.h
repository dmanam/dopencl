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
 * @file Memory.h
 *
 * @date 2011-08-21
 * @author Philipp Kegel
 */

#ifndef CL_MEMORY_H_
#define CL_MEMORY_H_

#include "Retainable.h"

#include "dclicd/detail/MappedMemory.h"

#include <dcl/ComputeNode.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>


class _cl_mem:
public _cl_retainable,
public dcl::Remote {
public:
    /**
     * @brief Finds a valid dclicd::Memory object for a given cl_mem object.
     *
     * @param[in] ptr   pointer to OpenCL memory object
     * @return memory object, if the given pointer points to a valid memory object, otherwise \code NULL
     */
    static cl_mem findMemObject(
            cl_mem ptr);


    virtual ~_cl_mem();

    cl_context context() const;
    void getInfo(
            cl_mem_info param_name,
            size_t      param_value_size,
            void *      param_value,
            size_t *    param_value_size_ret) const;

    /**
     * @brief Registers a user callback function with this memory object.
     */
    void setDestructorCallback(
            void (CL_CALLBACK * pfn_notify)(
                    cl_mem, void *),
            void *              user_data);

    /**
     * @brief Tests, if this memory objects is writable
     *
     * @return @c true, if this memory object is writeable, otherwise @c false
     */
    bool isOutput() const;

    /**
     * @brief Unmaps a previously mapped region of a memory object.
     *
     * This method only discards the pointer to mapped memory but it does not
     * unmap, i.e. copy data to a device.
     *
     * @param[in]  mappedPtr
     */
    virtual void unmap(
            void *mappedPtr) = 0;


    /**
     * @brief Callback for a completed acquire operation.
     *
     * NOTE: This method is a work-around for missing node-to-node communication.
     * When an acquire operation is completed which has been performed on behalf
     * of a compute node the acquired data is forwarded to the requesting
     * compute node.
     *
     * @param[in]  destination      the requesting compute node where the memory
     *                              object has to be send to as an update
     * @param[in] executionStatus
     */
    void onAcquireComplete(
            dcl::Process& destination,
            cl_int        executionStatus);

    /**
     * @brief Callback for acquiring this memory object's data on behalf of a compute node
     *
     * NOTE: This method is a work-around for missing node-to-node communication.
     * Rather than requesting a memory object's data from a compute node
     * directly, the data is requested from the host which acquires that data
     * from the compute node and forwards it to the requesting compute node.
     *
     * @param[in]  destination  the requesting compute node where the memory
     *                          object has to be send to as an update
     * @param[in]  source       the compute node owning the latest changes to
     *                          this memory object
     */
    void onAcquire(
            dcl::Process& destination,
            dcl::Process& source);

protected:
    _cl_mem(cl_context      context,
            cl_mem_flags    flags,
            size_t          size,
            void *          host_ptr);

    /**
     * @brief Allocates host memory for this memory object
     */
    void allocHostMemory();

    /**
     * @brief Frees host memory that has been allocated for this memory object
     */
    void freeHostMemory();

    /**
     * @brief Lock the pages holding this memory object in host memory.
     */
    void lockHostMemory();

    /**
     * @brief Unlock the pages holding this memory object in host memory.
     */
    void unlockHostMemory();

    /**
     * @brief Query memory object type.
     *
     * This method is reserved for internal use by @c _cl_mem::getInfo.
     *
     * @return the type of the command that is associated with this event
     */
    virtual cl_mem_object_type type() const = 0;

    virtual cl_uint mapCount() const = 0;

    virtual cl_mem associatedMemObject() const = 0;

    virtual size_t offset() const = 0;

    void destroy();

    /**
     * @brief Acquires this memory object's data from a process (compute node).
     *
     * This method updates this memory object's data by downloading the latest
     * bits from the specified compute node.
     *
     * @param[in]  owner    the process (a compute node) to acquire the memory
     *                      object's data from
     * @return a handle for the data transfer
     */
    std::shared_ptr<dcl::DataTransfer> acquire(
            dcl::Process& process);

    cl_context _context;
    cl_mem_flags _flags;
    size_t _size;
    void *_host_ptr; /**< host_ptr argument specified, when memory object has been created */
    void *_data; /**< a cached copy of this memory object's data, used, e.g., for mapping */

    mutable std::mutex _dataMutex; /**< Mutex for data; mostly used for mapping */

    /**
     * @brief Callbacks called when this memory object is destroyed.
     *
     * Access to this member cannot be synchronized in a sensible manner as a
     * mutex variable would be destroyed together with the memory object.
     */
    std::vector<std::pair<void (CL_CALLBACK *)(cl_mem, void *), void *>> _destructorCallbacks;

private:
    static std::set<cl_mem> _created_mem_obj; /**< list of valid memory objects */
    /* TODO Synchronize access to _cl_mem::_created_mem_obj */
};

#endif /* CL_MEMORY_H_ */
