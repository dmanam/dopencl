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
 * \file   CLObjectRegistry.h
 *
 * \date   2012-07-12
 * \author Philipp Kegel
 */

#ifndef CLOBJECTREGISTRY_H_
#define CLOBJECTREGISTRY_H_

#include "DCLTypes.h"

#include <map>

namespace dcl {

namespace detail {

template<class T>
class Registry {
public:
    void bind(
            object_id   id,
            T&          object);
    void unbind(
            object_id   id);
    T * lookup(
            object_id   id) const;

private:
    std::map<object_id, T *> _objects;
};

} /* namespace detail */

/* ****************************************************************************/

class CommandListener;
class CommandQueueListener;
class ContextListener;
class ProgramBuildListener;
class SynchronizationListener;

/* ****************************************************************************/

/*!
 * \brief A lookup facility for obtaining objects by their associated ID.
 *
 * It is the central resolver for object IDs.
 */
class CLObjectRegistry:
        private detail::Registry<CommandListener>,
        private detail::Registry<CommandQueueListener>,
        private detail::Registry<ContextListener>,
        private detail::Registry<ProgramBuildListener>,
        private detail::Registry<SynchronizationListener> {
public:
    CLObjectRegistry();
    virtual ~CLObjectRegistry();

    /*!
     * \brief Associates an ID with an object.
     *
     * \param[in]  id       the ID to assign
     * \param[in]  object   the object to assign
     */
    template<class T>
    void bind(object_id id, T& object) {
        detail::Registry<T>::bind(id, object);
    }

    /*!
     * \brief Delete the association of the specified ID.
     *
     * \param[in]  id   the ID which association should be deleted
     */
    template<class T>
    void unbind(object_id id) {
        detail::Registry<T>::unbind(id);
    }

    /*!
     * \brief Looks up the object that is associated with the specified ID.
     *
     * \param[in]  id   the ID of the object to look up
     * \return the object associated with ID, or \c NULL if no object is associated with id
     */
    template<class T>
    T * lookup(object_id id) const {
        return detail::Registry<T>::lookup(id);
    }
};

} /* namespace dcl */

#endif /* CLOBJECTREGISTRY_H_ */
