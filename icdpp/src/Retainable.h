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
 * @file    Retainable.h
 *
 * @date    2011-05-18
 * @author  Louay Hashim
 * @author  Philipp Kegel
 */

#ifndef CL_RETAINABLE_H_
#define CL_RETAINABLE_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <atomic>

class _cl_retainable {
public:
    /**
     * @brief Creates a retainable object.
     *
     * The object is retained implicitly, i.e., the reference counter is set to 1.
     */
    _cl_retainable();
    virtual ~_cl_retainable();

    /**
     * @brief Increases this object's reference count by one.
     */
    virtual void retain();

    /**
     * @brief Decreases this object's reference count by one.
     *
     * This object is destroyed, if its reference count becomes zero.
     *
     * @return @c true, if this object has been destroyed, otherwise @c false
     */
    virtual bool release();

protected:
    virtual void destroy() = 0;

    std::atomic<cl_uint> _ref_count;
};

#endif /* CL_RETAINABLE_H_ */
