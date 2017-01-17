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
 * \file Context.h
 *
 * \date 2011-04-06
 * \author Philipp Kegel
 */

#ifndef CL_CONTEXT_H_
#define CL_CONTEXT_H_

#include "Retainable.h"

#include "dclicd/detail/ContextProperties.h"

#include <dcl/ComputeNode.h>
#include <dcl/ContextListener.h>
#include <dcl/DCLTypes.h>
#include <dcl/Remote.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>


class _cl_context:
public _cl_retainable,
public dcl::Remote,
public dcl::ContextListener {
public:
    /*!
     * \brief Creates an OpenCL context.
     *
     * \param[in]  properties   context properties; may be NULL
     * \param[in]  devices      a list of unique devices returned by \c Platform::getDevices.
     *             Duplicate devices specified in \c devices are ignored.
     * \param[in]  pfn_notify   a callback function that can be registered by the application
     * \param[in]  user_data    passed as the \c user_data argument when \c pfn_notify is called.
     *             user_data can be \c NULL.
     */
    _cl_context(
            const dclicd::detail::ContextProperties *   properties,
            const std::vector<cl_device_id>&            devices,
            void (*                                     pfn_notify) (
                    const char *    errinfo,
                    const void *    private_info,
                    size_t          cb,
                    void *          user_data),
            void *                                      user_data);

    /*!
     * \brief Creates an OpenCL context from a device type that identifies the specific device(s) to use.
     *
     * \param[in]  properties   context properties; may be NULL
     * \param[in]  device_type	a bit-field that identifies the type of device
     * \param[in]  pfn_notify   a callback function that can be registered by the application
     * \param[in]  user_data    passed as the \c user_data argument when \c pfn_notify is called.
     *             \c user_data can be \c NULL.
     */
    _cl_context(
            const dclicd::detail::ContextProperties *   properties,
            cl_device_type                              device_type,
            void (*                                     pfn_notify)(
                    const char *    errinfo,
                    const void *    private_info,
                    size_t          cb,
                    void *          user_data),
            void *                                      user_data);

    /*!
     * \brief Creates an OpenCL context from all devices of the specified compute nodes.
     *
     * \param[in]  properties       context properties; may be NULL
     * \param[in]  compute_nodes	a list of compute nodes
     * \param[in]  pfn_notify       a callback function that can be registered by the application
     * \param[in]  user_data        passed as the \c user_data argument when \c pfn_notify is called.
     *             \c user_data can be \c NULL.
     */
    _cl_context(
            const dclicd::detail::ContextProperties *   properties,
            const std::vector<cl_compute_node_WWU>&     computeNodes,
            void (*                                     pfn_notify)(
                    const char *    errinfo,
                    const void *    private_info,
                    size_t          cb,
                    void *          user_data),
            void *                                      user_data);

    virtual ~_cl_context();

    const std::vector<dcl::ComputeNode *>& computeNodes() const;

    /*!
     * \brief Returns the platform associated with context.
     *
     * This platform either has been specified in context properties, or is a
     * default platform that has been selected by the implementation.
     *
     * \return a platform
     */
    cl_platform_id getPlatform() const;

    /*!
     * \brief Returns the devices associated with this context.
     *
     * This method is an efficient and convenient alternative for getInfo.
     *
     * \return a list of devices
     */
    const std::vector<cl_device_id>& devices() const;
    void getInfo(
            cl_context_info param_name,
            size_t          param_value_size,
            void *          param_value,
            size_t *        param_value_size_ret) const;

    void getSupportedImageFormats(
            cl_mem_flags                    flags,
            cl_mem_object_type              image_type,
            std::vector<cl_image_format>&   imageFormats) const;

    bool hasDevice(
            cl_device_id device) const;

    /*
     * Context listener APIs
     */
    void onError(
            const char *errorInfo,
            const void *private_info,
            size_t      cb);

protected:
    void destroy();

private:
    /*!
     * \brief Initializes a newly created context from a list of devices.
     *
     * \param[in]  devices  a list of devices that will associated with the context
     */
    void init(
            const std::vector<cl_device_id>&);

    std::vector<dcl::ComputeNode *> _computeNodes; //!< compute nodes hosting this context

    std::unique_ptr<dclicd::detail::ContextProperties> _properties;
    std::vector<cl_device_id> _devices;
    void (*_pfnNotify)(
            const char *errinfo,
            const void *private_info,
            size_t cb,
            void *user_data);
    void *_userData;
};

#endif /* CL_CONTEXT_H_ */
