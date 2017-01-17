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
 * \file    Kernel.h
 *
 * \date    2011-06-08
 * \author  Karsten Jeschkies
 * \author  Philipp Kegel
 */

#ifndef CL_KERNEL_H_
#define CL_KERNEL_H_

#include "Retainable.h"

#include <dcl/Binary.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <vector>


class _cl_kernel: public _cl_retainable, public dcl::Remote {
public:
    /*!
     * \brief  Creates kernel objects for all kernel functions in program.
     *
     * Kernel objects are not created for any \c __kernel functions in program
     * that do not have the same function definition across all devices for
     * which a program executable has been successfully built.
     *
     * \param[in]  program  the program from which the kernels should be created
     * \param[in]  kernels  a list of kernels created from the program
     */
    static void createKernelsInProgram(
            cl_program              program,
            std::vector<cl_kernel>& kernels);


    _cl_kernel(
            cl_program  program,
            const char *kernelName);
    virtual ~_cl_kernel();

    cl_program program() const;
    void getInfo(
            cl_kernel_info  param_name,
            size_t          param_value_size,
            void *          param_value,
            size_t *        param_value_size_ret) const;

#if defined(CL_VERSION_1_2)
    void getArgInfo(
            cl_uint             arg_indx,
            cl_kernel_arg_info  param_name,
            size_t              param_value_size,
            void *              param_value,
            size_t *            param_value_size_ret) const;
#endif // #if defined(CL_VERSION_1_2)

    void getWorkGroupInfo(
            cl_device_id                device,
            cl_kernel_work_group_info   param_name,
            size_t                      param_value_size,
            void *                      param_value,
            size_t *                    param_value_size_ret) const;

    /*!
     * \brief Sets a kernel argument.
     *
     * \param[in]  index    index of the argument
     * \param[in]  size     size of the argument value
     * \param[in]  value    pointer to the argument value.
     * Value may be NULL for memory objects and must be \c NULL for arguments
     * for which the \c __local qualifier is specified.
     */
    void setArgument(
            cl_uint     index,
            size_t      size,
            const void *value = nullptr);

    /**
     * @brief Returns the memory objects (possibly) written to with this kernel
     *
     * @return a list of memory objects
     */
    std::vector<cl_mem> writeMemoryObjects() const;

protected:
    void destroy();

private:
    /*!
     * \brief Private constructor setting ID of kernel object.
     *
     * Make sure that no kernel objects share the same ID!
     * NOTE: This constructor does NOT create kernels on any compute node.
     * Basically this constructor just sets all member variables.
     *
     * \param[in]  id       kernel ID
     * \param[in]  program  program associated with kernel; must not be \c NULL
     */
    _cl_kernel(
            dcl::object_id  id,
            cl_program      program);

    cl_program _program;

    /** Kernel info cache */
    mutable std::map<cl_kernel_info, dcl::Binary> _infoCache;
    /** Work group info caches */
    mutable std::map<cl_device_id, std::map<cl_kernel_work_group_info, dcl::Binary>> _workGroupInfoCaches;
    mutable std::mutex _infoCacheMutex;

    /**
     * @brief Memory objects modified by this kernel
     */
    std::vector<cl_mem> _writeMemoryObjects;
};

#endif /* CL_KERNEL_H_ */
