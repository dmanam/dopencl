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
 * \file Error.cpp
 *
 * \date 2011-02-06
 * \author Philipp Kegel
 */

#include "Error.h"

#include <dcl/CLError.h>
#include <dcl/DCLException.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <string>

namespace dclicd {

Error::Error(cl_int err, const char *what) throw () :
	_err(err)
{
	if (what) {
		/* a string must not be initialized with NULL */
		_what.assign(what);
	}
}

Error::Error(cl_int err, const std::string& what) throw () :
	_err(err), _what(what) { }

Error::Error(const dcl::CLError& err) throw() :
	_err(err.err()), _what(err.what()) { }

Error::Error(const dcl::ConnectionException& err) throw() :
	_err(CL_CONNECTION_ERROR_WWU), _what(err.what()) { }

Error::Error(const dcl::IOException& err) throw() :
	_err(CL_IO_ERROR_WWU), _what(err.what()) { }

Error::Error(const dcl::ProtocolException& err) throw() :
	_err(CL_PROTOCOL_ERROR_WWU), _what(err.what()) { }

Error::~Error() throw() { }

const char * Error::what() const throw() {
	return _what.c_str();
}

cl_int Error::err() const throw() {
	return _err;
}

} /* namespace dclicd */
