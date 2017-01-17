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
 * \file   CLObjectRegistry.cpp
 *
 * \date   2012-07-31
 * \author Philipp Kegel
 */

#include <dcl/CLObjectRegistry.h>

#include <dcl/CommandListener.h>
#include <dcl/CommandQueueListener.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLTypes.h>
#include <dcl/ProgramBuildListener.h>
#include <dcl/SynchronizationListener.h>

#include <iterator>
#include <map>
#include <utility>

namespace dcl {

namespace detail {

template<class T>
void Registry<T>::bind(object_id id, T& object) {
    _objects.insert(std::make_pair(id, &object));
}

template<class T>
void Registry<T>::unbind(object_id id) {
    _objects.erase(id);
}

template<class T>
T * Registry<T>::lookup(object_id id) const {
    auto i = _objects.find(id);
    return (i == std::end(_objects)) ? nullptr : i->second;
}

/* explicit instantiation */
template class Registry<CommandListener>;
template class Registry<CommandQueueListener>;
template class Registry<ContextListener>;
template class Registry<ProgramBuildListener>;
template class Registry<SynchronizationListener>;
/* ^^^ add support for additional object types in registry here ^^^ */

} /* namespace detail */

/* ****************************************************************************/

CLObjectRegistry::CLObjectRegistry() {
}

CLObjectRegistry::~CLObjectRegistry() {
}

} /* namespace dcl */
