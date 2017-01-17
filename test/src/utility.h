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
 * \file utility.h
 *
 * \date 2013-12-08
 * \author Philipp Kegel
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <vector>

namespace dcltest {

extern const char *source; //!< example source code
extern const char *source1; //!< example source code 1
extern const char *source2; //!< example source code 2

/*!
 * \brief Returns the dOpenCL platform
 *
 * \return the dOpenCL platform
 */
cl_platform_id getPlatform();

/*!
 * \brief Obtains the specified number of compute nodes from the given platform.
 *
 * \param[in]  platform
 * \param[in]  num_nodes
 * \param[out] nodes
 */
void getComputeNodes(
        cl_platform_id platform,
        cl_uint num_nodes,
        cl_compute_node_WWU *nodes);

/*!
 * \brief Obtains the specified number of compute nodes from the given platform.
 *
 * \param[in]  platform     the platform
 * \param[in]  tyoe         type of devices to obtain
 * \param[in]  num_devices  number of devices to obtain
 * \param[out] devices      the devices
 */
void getDevices(
        cl_platform_id platform,
        cl_device_type type,
        cl_uint num_devices,
        cl_device_id *devices);

/*!
 * \brief Returns the specified platform's first device.
 *
 * \param[in]  platform the platform
 * \return the platform's first device
 */
cl_device_id getDevice(
        cl_platform_id platform);

/*!
 * \brief Returns the specified compute node's first device.
 *
 * \param[in]  node the compute node
 * \return the compute node's first device
 */
cl_device_id getDevice(
        cl_compute_node_WWU node);

cl_context createContext(
        cl_uint num_devices,
        cl_device_id *devices);

cl_command_queue createCommandQueue(
        cl_context context,
        cl_device_id device,
        cl_command_queue_properties properties = 0);

cl_program createProgramWithSource(
        cl_context context,
        cl_uint num_sources,
        const char **sources,
        size_t *lengths = nullptr);

cl_mem createRWBuffer(
        cl_context context,
        size_t cb);

/* ****************************************************************************/

/*!
 * \brief Fills a vector with values
 *
 * \param[out] vec      vector to fill
 * \param[in]  first    first value
 * \param[in]  step     steps between consecutive values
 */
template<typename T>
void fillVector(std::vector<T>& vec, T first, T step) {
    T value = first;
    for (auto& item : vec) {
        item = value;
        value += step;
    }
}

} // namespace dcltest

#endif /* UTILITY_H_ */
