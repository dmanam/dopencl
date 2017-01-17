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
 * \file    DCLException.h
 *
 * \date    2012-02-12
 * \author  Philipp Kegel
 */

#ifndef DCLEXCEPTION_H_
#define DCLEXCEPTION_H_

#include <exception>
#include <string>

/* Error codes */
#define DCL_SUCCESS                           0
#define DCL_CONNECTION_ERROR              -2001
#define DCL_IO_ERROR                      -2002
#define DCL_PROTOCOL_ERROR                -2003
#define DCL_INVALID_CONNECTION_MANAGER    -2051
#define DCL_INVALID_HOST                  -2052
#define DCL_INVALID_CONNECTION_LISTENER   -2053
#define DCL_INVALID_COMMAND_LISTENER      -2054
#define DCL_INVALID_DEVICE_MANAGER        -2055
#define DCL_INVALID_NODE                  -2056

/* ****************************************************************************/

namespace dcl {

class DCLException : public std::exception {
public:
	DCLException(
    		const char *what = nullptr) throw ();
	DCLException(
    		const std::string& what) throw ();
    virtual ~DCLException() throw();

    /*!
     * \brief Gets the error string associated with this exception.
     *
     * \return a memory pointer to the error message string.
     */
    virtual const char * what() const throw ();

protected:
    std::string _what;
};

/* ****************************************************************************/

template<int N>
class BasicException: public DCLException {
public:
	static const int Type = N;

	BasicException(
    		const char *what = nullptr) throw ();
	BasicException(
    		const std::string& what) throw ();
};

typedef BasicException<DCL_CONNECTION_ERROR> ConnectionException;
typedef BasicException<DCL_IO_ERROR> IOException;
typedef BasicException<DCL_PROTOCOL_ERROR> ProtocolException;

/* ****************************************************************************/

class InvalidArgument: public DCLException {
public:
	InvalidArgument(
			int /* err */,
    		const char *what = nullptr) throw ();
	InvalidArgument(
			int /* err */,
    		const std::string& /* what */) throw ();
    virtual ~InvalidArgument() throw();

    /*!
     * \brief Gets the error code associated with this exception.
     *
     * \return the error code
     */
    int err() const throw();

protected:
    int _err;
};

/* ****************************************************************************/

/*!
 * \brief A class to indicate an interrupted blocking thread
 */
class ThreadInterrupted : public DCLException {
public:
    ThreadInterrupted(
            const char *what = nullptr) throw ();
    ThreadInterrupted(
            const std::string& what) throw ();
    virtual ~ThreadInterrupted() throw();
};

} /* DCLEXCEPTION_H_ */

#endif /* DCLERROR_H_ */
