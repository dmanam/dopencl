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
 * \file SmartCLObjectRegistry.h
 *
 * An implementation of an object registry.
 * This class is similar to CLObjectRegistry but provides special handling for smart pointers.
 * It does not hold ownership of registered objects but uses either pointers or weak pointers (for smart pointer) internally.
 * This type of registry is used only on the compute node side, where the Session class own all objects.
 * On the host side, this class cannot be used currently as there is no object owner.
 *
 * \date 2013-10-22
 * \author Philipp Kegel
 */

#ifndef SMARTCLOBJECTREGISTRY_H_
#define SMARTCLOBJECTREGISTRY_H_

#include <dcl/Context.h>
#include <dcl/CommandQueue.h>
#include <dcl/DCLTypes.h>
#include <dcl/Device.h>
#include <dcl/Event.h>
#include <dcl/Kernel.h>
#include <dcl/Memory.h>
#include <dcl/Program.h>

#include <iterator>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace dclasio {

namespace detail {

template<typename T>
class RegistryValue {
public:
    typedef T Type;

    static Type put(T& value) { return value; }
    static T get(Type value) { return value; }
};

template<typename T>
class RegistryValue<std::shared_ptr<T>> {
public:
    typedef std::weak_ptr<T> Type;

    static Type put(std::shared_ptr<T>& value) { return std::weak_ptr<T>(value); }
    static std::shared_ptr<T> get(const Type& value) { return value.lock(); }
};

/* ****************************************************************************/

template<typename T>
class Registry {
public:
    void bind(
            dcl::object_id id,
            T& objectPtr) {
        _objects.insert(std::make_pair(id, RegistryValue<T>::put(objectPtr)));
    }

    void unbind(
            dcl::object_id id) {
        _objects.erase(id);
    }

    T lookup(
            dcl::object_id id) const {
        auto i = _objects.find(id);
        return (i == std::end(_objects)) ? T() : RegistryValue<T>::get(i->second);
    }

    void getIDs(
            std::vector<dcl::object_id>& ids) const {
        ids.clear();
        for (auto entry : _objects) {
            ids.push_back(entry.first);
        }
    }

private:
    std::map<dcl::object_id, typename RegistryValue<T>::Type> _objects;
};

} /* namespace detail */

/* ****************************************************************************/

/*!
 * \brief A lookup facility for obtaining objects by their associated ID.
 *
 * It is the central resolver for object IDs.
 */
class SmartCLObjectRegistry:
        private detail::Registry<std::shared_ptr<dcl::Buffer>>,
        private detail::Registry<std::shared_ptr<dcl::Context>>,
        private detail::Registry<std::shared_ptr<dcl::CommandQueue>>,
        private detail::Registry<dcl::Device *>,
        private detail::Registry<std::shared_ptr<dcl::Event>>,
        private detail::Registry<std::shared_ptr<dcl::Kernel>>,
        private detail::Registry<std::shared_ptr<dcl::Program>> {
public:
    SmartCLObjectRegistry();
    virtual ~SmartCLObjectRegistry();

    /*!
     * \brief Associates an ID with an object.
     *
     * \param[in]  id           the ID to assign
     * \param[in]  objectPtr    the object to assign
     */
    template<typename T>
    void bind(
            dcl::object_id id,
            T& objectPtr) {
        detail::Registry<T>::bind(id, objectPtr);
    }

    /*!
     * \brief Delete the association of the specified ID.
     *
     * \param[in]  id   the ID which association should be deleted
     */
    template<typename T>
    void unbind(dcl::object_id id) {
        detail::Registry<T>::unbind(id);
    }

    void unbindMemory(
            dcl::object_id id);

    /*!
     * \brief Looks up the object that is associated with the specified ID.
     *
     * \param[in]  id   the ID of the object to look up
     * \return the object associated with ID, or \c NULL if no object is associated with id
     */
    template<typename T>
    T lookup(dcl::object_id id) const {
        return detail::Registry<T>::lookup(id);
    }

    template<typename T>
    void lookup(
            const std::vector<dcl::object_id>& ids,
            std::vector<T>& objects) const {
        objects.clear();
        objects.reserve(ids.size());

        for (auto id : ids) {
            objects.push_back(detail::Registry<T>::lookup(id));
        }
    }

    std::shared_ptr<dcl::Memory> lookupMemory(
            dcl::object_id id) const;

    template<typename T>
    void getIDs(
            std::vector<dcl::object_id>& ids) const {
        detail::Registry<T>::getIDs(ids);
    }
};

} /* namespace dclasio */

#endif /* SMARTCLOBJECTREGISTRY_H_ */
