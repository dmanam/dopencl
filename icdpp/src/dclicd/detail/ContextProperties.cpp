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
 * \file ContextProperties.cpp
 *
 * \date 2012-08-02
 * \author Philipp Kegel
 */

#include "ContextProperties.h"

#include "../../Platform.h"

#include "../Error.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <algorithm>
#include <iterator>
#include <cassert>
#include <cstddef>
#include <set>
#include <stdexcept>

namespace dclicd {

namespace detail {

ContextProperties::ContextProperties(
        const cl_context_properties *properties) {
    init(properties, &_size, &_properties);
}

ContextProperties::ContextProperties(
        const ContextProperties& rhs) :
    _size(rhs._size)
{
    /* Deep copy properties list */
    try {
        _properties = new cl_context_properties[_size];
        std::copy(rhs._properties, rhs._properties + _size,
                _properties);
    } catch (const std::bad_alloc&) {
        throw Error(CL_OUT_OF_HOST_MEMORY);
    }
}

ContextProperties::~ContextProperties() {
    delete _properties;
}

ContextProperties& ContextProperties::operator=(
        ContextProperties& rhs) {
    if (&rhs != this) {
        delete _properties; // discard current properties

        _size = rhs._size;
        _properties = rhs._properties;

        rhs._size = 0;
        rhs._properties = nullptr;
    }

    return *this;
}

ContextProperties& ContextProperties::operator=(
        const cl_context_properties *properties) {
    size_t size;
    cl_context_properties *newProperties;

    init(properties, &size, &newProperties);
    delete _properties; // discard current properties
    _size = size;
    _properties = newProperties;

    return *this;
}

size_t ContextProperties::size() const {
    return _size;
}

size_t ContextProperties::numProperties() const {
    return _size / 2;
}

template<cl_context_properties Property, typename T>
T ContextProperties::property() const {
    cl_context_properties *property = _properties;
    auto value = static_cast<T>(0x0); // assume default value

    while (*property) {
        auto name = *property++;

        if (name == Property) {
            value = reinterpret_cast<T>(*property);
            break;
        }
        property++; // skip property value
    }

    return value;
}

/* explicit instantiation */
template cl_platform_id ContextProperties::property<CL_CONTEXT_PLATFORM, cl_platform_id>() const;

const cl_context_properties * ContextProperties::data() const {
    return _properties;
}

void ContextProperties::init(const cl_context_properties *properties,
        size_t *size_ret, cl_context_properties **properties_ret) {
    size_t size = 0;

    assert(size_ret != nullptr);
    assert(properties_ret != nullptr);

    if (properties) {
        std::set<cl_context_properties> propertyNames;
        const cl_context_properties *property = properties;

        /*
         * Validate property list and determine size
         */
        /* Count number of properties */
        while (*property) { // property name NULL terminates list
            auto name = *property++;

            if (!propertyNames.insert(name).second) {
                /* property name only be specified once */
                throw Error(CL_INVALID_PROPERTY);
            }

            /* Properties other than CL_CONTEXT_PLATOFRM are *not* checked as
             * this should be left to the native platforms on the compute nodes
             * when creating a context with these properties. */
            switch (name) {
            case CL_CONTEXT_PLATFORM:
            {
                std::vector<cl_platform_id> platforms;

                _cl_platform_id::get(platforms);

                if (std::find(std::begin(platforms), std::end(platforms),
                        reinterpret_cast<cl_platform_id>(*property)) == std::end(platforms)) {
                    throw Error(CL_INVALID_PLATFORM);
                }
            }
                break;
//            case CL_CONTEXT_D3D10_DEVICE_KHR:
//                assert(!"cl_khr_d3d10_sharing not supported by dOpenCL");
//                break;
//            case CL_GL_CONTEXT_KHR:
//            case CL_EGL_DISPLAY_KHR:
//            case CL_GLX_DISPLAY_KHR:
//            case CL_WGL_HDC_KHR:
//            case CL_CGL_SHAREGROUP_KHR:
//                assert(!"cl_khr_gl_sharing not supported by dOpenCL");
//                break;
            default:
                throw Error(CL_INVALID_PROPERTY);
            }

            size += 2;  // count name and value
            ++property; // skip property value
        }

        ++size; // count terminating NULL;
    } else {
        throw std::invalid_argument("Context properties must not be NULL");
    }

    /* Deep copy properties list */
    try {
        *properties_ret = new cl_context_properties[size];
        std::copy(properties, properties + size, *properties_ret);
    } catch (const std::bad_alloc&) {
        throw Error(CL_OUT_OF_HOST_MEMORY);
    }

    *size_ret = size;
}

} /* namespace detail */

} /* namespace dclicd */
