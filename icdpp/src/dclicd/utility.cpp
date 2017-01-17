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
 * \file utility.cpp
 *
 * \date 2012-02-28
 * \author Philipp Kegel
 */

#include "utility.h"

#include "../ComputeNode.h"

#include "Error.h"
#include <dcl/Binary.h>

#ifdef __APPLE__
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl_wwu_dcl.h>
#endif

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

namespace dclicd {

void copy_info(size_t size, const void *value, size_t param_value_size,
		void *param_value, size_t *param_value_size_ret) {
	if (param_value) {
		if (size > param_value_size) {
			throw Error(CL_INVALID_VALUE);
		}

		/* copy parameter value */
		memcpy(param_value, value, size);
	}

	if (param_value_size_ret) {
		*param_value_size_ret = size;
	}
}

template<>
void copy_info(
		const std::string& param,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) {
    /* For strings, value_size is string length plus 1 for trailing \0 character. */
    copy_info(param.length() + 1, static_cast<const void *>(param.c_str()),
            param_value_size, param_value, param_value_size_ret);
}

template<>
void copy_info(
		const dcl::Binary& param,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret) {
	copy_info(param.size(), param.value(), param_value_size, param_value,
			param_value_size_ret);
}

/******************************************************************************/

template<>
void release(cl_compute_node_WWU compute_node) {
    assert(compute_node != nullptr);
    compute_node->release();
}

} /* namespace dclicd */

