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
 * \file ContextProperties.h
 *
 * \date 2012-08-02
 * \author Philipp Kegel
 */

#ifndef CONTEXTPROPERTIES_H_
#define CONTEXTPROPERTIES_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <cstddef>

namespace dclicd {

namespace detail {

/*!
 * \brief A wrapper for an array of context properties.
 */
class ContextProperties {
public:
    ContextProperties(
            const cl_context_properties *properties);
    ContextProperties(
            const ContextProperties& rhs);
    virtual ~ContextProperties();

    /*!
     * \brief A move assignment operator for context properties.
     */
    ContextProperties& operator=(
            ContextProperties& rhs);
    ContextProperties& operator=(
            const cl_context_properties *properties);

    /*!
     * \brief Returns the size of this list of context properties.
     *
     * The size is the number of entries including the terminating \c NULL entry.
     *
     * \return the size of this property list
     */
    size_t size() const;

    /*!
     * \brief Returns the number of properties, i.e., name-value pairs in this list of context properties.
     *
     * \return the number of properties
     */
    size_t numProperties() const;

    template<cl_context_properties Property, typename T>
    T property() const;

    const cl_context_properties * data() const;

private:
    static void init(
            const cl_context_properties *properties,
            size_t *                     size_ret,
            cl_context_properties **     properties_ret);

    size_t _size;
    cl_context_properties *_properties;
};

} /* namespace detail */

} /* namespace dclicd */

#endif /* CONTEXTPROPERTIES_H_ */
