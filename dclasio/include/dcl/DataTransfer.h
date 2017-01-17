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
 * \file    DataTransfer.h
 *
 * \date    2011-12-17
 * \author  Philipp Kegel
 *
 * C++ API declarations for dOpenCL communication layer
 */

#ifndef DATATRANSFER_H_
#define DATATRANSFER_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <functional>

namespace dcl {

/*!
 * \brief A handle for an asynchronous data transfer.
 */
class DataTransfer {
public:
	virtual ~DataTransfer() { }

	/*!
	 * \brief Registers a callback which is called upon completion (or failure)
	 *        of this data transfer.
	 *
	 * \param[in]  notify   the callback to register
	 */
    virtual void setCallback(
            const std::function<void (cl_int)>& notify) = 0;

	virtual unsigned long submit() const = 0;
	virtual unsigned long start() const = 0;
	virtual unsigned long end() const = 0;

    virtual bool isComplete() const = 0;

	/*!
	 * \brief Blocks until this data transfer is complete.
	 */
	virtual void wait() const = 0;

	/*!
	 * \brief Aborts this data transfer.
	 *
	 * The data transfer is considered as failed after calling this method.
	 * All registered callbacks are called accordingly.
	 */
	virtual void abort() = 0;
};

} /* namespace dcl */

#endif /* DATATRANSFER_H_ */
