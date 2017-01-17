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
 * \file    Context.h
 *
 * \date    2012-07-07
 * \author  Philipp Kegel
 *
 * dOpenCL context API
 */

#ifndef DCL_CONTEXT_H_
#define DCL_CONTEXT_H_

#if 0
#include "DCLTypes.h"

#include <cstddef>
#include <string>
#include <vector>

#endif

namespace dcl {

class ComputeNode;

/* ****************************************************************************/

/*!
 * \brief Remote interface of a context
 *
 * The context interface it used by both host and compute nodes, but only
 * compute nodes should call the sole method Context::raiseError of this
 * interface, while only the host should implement this method.
 */
class Context {
public:
    virtual ~Context() { }

#if 0
    virtual void raiseError(
            const std::string&  errorInfo,
            const void *        private_info,
            size_t              cb) = 0;

    /* TODO Delete dcl::Context::getId when all remote ICD objects are accessible through interface classes. */
    virtual dcl::object_id getId() const = 0;
    /* TODO Delete dcl::Context::getComputeNodes when all remote ICD objects are accessible through interface classes. */
    virtual void getComputeNodes(
            std::vector<ComputeNode *>& computeNodes) const = 0;
#endif
};

} /* namespace dcl */

#endif /* DCL_CONTEXT_H_ */
