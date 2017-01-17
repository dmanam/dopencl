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
 * \file utility.h
 *
 * \date 2012-02-28
 * \author Philipp Kegel
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <dcl/Binary.h>

#ifdef __APPLE__
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl_wwu_dcl.h>
#endif

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace dclicd {

/*!
 * \brief Copy information at source pointer to destination pointer.
 *
 * This function implements the most generic approach for copying information.
 * It should be avoided in favor of the templated typed versions of copy_info.
 *
 * \param[in]  size             size of source
 * \param[in]  value            source
 * \param[in]  param_value_size size of destination
 * \param[out] param_value      destination
 * \param[out] param_value_ret  size of source
 */
void copy_info(
		size_t      size,
		const void *value,
		size_t      param_value_size,
		void *      param_value,
		size_t *    param_value_size_ret);

template<typename T>
void copy_info(
		const T& param,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) {
	copy_info(sizeof(T), static_cast<const void *>(&param),
			param_value_size, param_value, param_value_size_ret);
}

template<typename T>
void copy_info(
        const std::vector<T>& param,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret) {
    copy_info(sizeof(T) * param.size(),
            static_cast<const void *>(param.data()), param_value_size,
            param_value, param_value_size_ret);
}

template<>
void copy_info(
		const std::string& param,
		size_t             param_value_size,
		void *             param_value,
		size_t *           param_value_size_ret);

template<>
void copy_info(
		const dcl::Binary& param,
		size_t             param_value_size,
		void *             param_value,
		size_t *           param_value_size_ret);


/******************************************************************************/

/**
 * @brief Releases and possibly deletes a retainable object.
 *
 * The object must have been created using operator new.
 *
 * @param[in]  object   the object to release. object must not be NULL.
 */
template<typename T>
void release(T *object) {
    assert(object != nullptr); // caller should return appropriate CL_INVALID_... error code
    /* FIXME Ensure that object has been created with new */
    if (object->release()) delete object;
}

/*
 * Compute nodes are deleted in _cl_platform::destroyComputeNode, such that
 * there must not be a redundant delete in dclicd::release.
 */
template<>
void release(cl_compute_node_WWU compute_node);

} /* namespace dclicd */

#endif /* UTILITY_H_ */
