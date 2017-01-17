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
 * @file cl.cpp
 *
 * @date 2011-04-12
 * @author Philipp Kegel
 *
 * @brief Implementation of the OpenCL API
 *
 * Function calls of the C API are redirected to corresponding method of a C++
 * implementation. Functions in this file only perform type conversions and
 * related operations, e.g. validating list parameters that are converted into
 * vectors. The C++ methods validate parameters.
 */

#include "CommandQueue.h"
#include "ComputeNode.h"
#include "Context.h"
#include "Device.h"
#include "Event.h"
#include "Kernel.h"
#include "Memory.h"
#include "Platform.h"
#include "Program.h"
#include "Retainable.h"

#include "dclicd/Buffer.h"
#include "dclicd/Error.h"
#include "dclicd/Event.h"
#include "dclicd/utility.h"

#include "dclicd/detail/ContextProperties.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h>
#include <OpenCL/cl_wwu_collective.h>
#include <OpenCL/cl_wwu_dcl.h>
#else
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_wwu_collective.h>
#include <CL/cl_wwu_dcl.h>
#endif

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


#if !defined(CL_VERSION_1_2)
/* Declare OpenCL 1.2 APIs as local helper functions.
 * These APIs are used internally to implement OpenCL 1.1 APIs (see end of this file). */
cl_int clEnqueueMarkerWithWaitList(
        cl_command_queue command_queue,
        cl_uint          num_events_in_wait_list,
        const cl_event * event_wait_list,
        cl_event *       event);

cl_int clEnqueueBarrierWithWaitList(
        cl_command_queue command_queue,
        cl_uint          num_events_in_wait_list,
        const cl_event * event_wait_list,
        cl_event *       event);

void * clGetExtensionFunctionAddressForPlatform(
        cl_platform_id platform,
        const char *   func_name);
#endif // #if !defined(CL_VERSION_1_2)

namespace {

template<typename CLObject> struct ErrorCode;
template<> struct ErrorCode<cl_device_id> { static const cl_int INVALID_OBJECT = CL_INVALID_DEVICE; };
template<> struct ErrorCode<cl_platform_id> { static const cl_int INVALID_OBJECT = CL_INVALID_PLATFORM; };
template<> struct ErrorCode<cl_context> { static const cl_int INVALID_OBJECT = CL_INVALID_CONTEXT; };
template<> struct ErrorCode<cl_command_queue> { static const cl_int INVALID_OBJECT = CL_INVALID_COMMAND_QUEUE; };
template<> struct ErrorCode<cl_mem> { static const cl_int INVALID_OBJECT = CL_INVALID_MEM_OBJECT; };
template<> struct ErrorCode<cl_program> { static const cl_int INVALID_OBJECT = CL_INVALID_PROGRAM; };
template<> struct ErrorCode<cl_kernel> { static const cl_int INVALID_OBJECT = CL_INVALID_KERNEL; };
template<> struct ErrorCode<cl_event> { static const cl_int INVALID_OBJECT = CL_INVALID_EVENT; };

template<typename CLObject>
inline cl_int clRetain(CLObject object) {
    if (!object)
        return ErrorCode<CLObject>::INVALID_OBJECT;

    object->retain();
    return CL_SUCCESS;
}

template<typename CLObject>
inline cl_int clRelease(CLObject object) {
    if (!object)
        return ErrorCode<CLObject>::INVALID_OBJECT;

    try {
        dclicd::release(object);
        return CL_SUCCESS;
    } catch (const dclicd::Error& err) {
        return err.err();
    }
}

template<typename CLObject>
inline cl_int clGetInfo(
        CLObject object, cl_context_info param_name,
        size_t param_value_size, void *param_value,
        size_t *param_value_size_ret) {
    if (!object)
        return ErrorCode<CLObject>::INVALID_OBJECT;

    try {
        object->getInfo(param_name, param_value_size, param_value,
                param_value_size_ret);
        return CL_SUCCESS;
    } catch (const dclicd::Error& err) {
        return err.err();
    }
}

} // anonymous namespace

/******************************************************************************/

/* Platform APIs */
cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms,
		cl_uint *num_platforms) {
	if (platforms) {
		if (num_entries == 0) return CL_INVALID_VALUE;
	} else {
		if (!num_platforms) return CL_INVALID_VALUE;
	}

	try {
		std::vector<cl_platform_id> platforms_;

		_cl_platform_id::get(platforms_);

		if (platforms) {
		    auto platform = std::begin(platforms_);
			for (cl_uint i = 0; i < num_entries && platform != std::end(platforms_); ++i) {
				*platforms++ = *platform++;
			}
		}
		if (num_platforms) *num_platforms = platforms_.size();
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

cl_int clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
		size_t param_value_size, void *param_value,
		size_t *param_value_size_ret) {
    return clGetInfo(
            platform ? platform : _cl_platform_id::dOpenCL(),
            param_name, param_value_size, param_value, param_value_size_ret);
}

/* Device APIs */
cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type,
		cl_uint num_entries, cl_device_id *devices, cl_uint *num_devices) {
	if (platform) {
		/* TODO Validate platform */
	} else {
		/* Behavior is implementation defined, if platform is NULL */
		platform = _cl_platform_id::dOpenCL();
	}

	if (devices) {
		if (num_entries == 0) return CL_INVALID_VALUE;
	} else {
		if (!num_devices) return CL_INVALID_VALUE;
	}

	try {
		std::vector<cl_device_id> devices_;

		platform->getDevices(device_type, devices_);

		if (devices) {
		    auto device = std::begin(devices_);
			for (cl_uint i = 0; i < num_entries && device != std::end(devices_); ++i) {
				*devices++ = *device++;
			}
		}
		if (num_devices) *num_devices = devices_.size();
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

cl_int clGetDeviceInfo(cl_device_id device, cl_device_info param_name,
		size_t param_value_size, void *param_value,
		size_t *param_value_size_ret) {
	return clGetInfo(device, param_name, param_value_size, param_value, param_value_size_ret);
}

#if defined(CL_VERSION_1_2)
cl_int clCreateSubDevices(
        cl_device_id in_device,
        const cl_device_partition_property *properties,
        cl_uint num_devices,
        cl_device_id *out_devices,
        cl_uint *num_devices_ret) {
    assert(!"clCreateSubDevices not implemented");
    return CL_SUCCESS;
}

cl_int clRetainDevice(cl_device_id device) {
    assert(!"clRetainDevice not implemented");
    return CL_SUCCESS;
}

cl_int clReleaseDevice(cl_device_id device) {
    assert(!"clReleaseDevice not implemented");
    return CL_SUCCESS;
}
#endif // #if defined(CL_VERSION_1_2)

/* Context APIs */
cl_context clCreateContext(const cl_context_properties *properties,
		cl_uint num_devices, const cl_device_id *devices, void(*pfn_notify)(
				const char *errinfo, const void *private_info, size_t cb,
				void *user_data), void *user_data, cl_int *errcode_ret) {
	cl_context context = nullptr;
	cl_int errcode = CL_SUCCESS;

	/*
	 * Validate device list
	 */
	if (num_devices == 0 || !devices) {
		errcode = CL_INVALID_VALUE;
	}

	/*
	 * Create context
	 */
	if (errcode == CL_SUCCESS) {
		try {
		    std::unique_ptr<dclicd::detail::ContextProperties> properties_;
		    if (properties) properties_.reset(new dclicd::detail::ContextProperties(properties));

			context = new _cl_context(properties_.get(),
			        std::vector<cl_device_id>(devices, devices + num_devices),
			        pfn_notify, user_data);
		} catch (const dclicd::Error& err) {
			errcode = err.err();
		} catch (const std::bad_alloc&) {
			errcode = CL_OUT_OF_HOST_MEMORY;
		}
	}

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return context;
}

cl_context clCreateContextFromType(const cl_context_properties *properties,
		cl_device_type device_type, void(*pfn_notify)(const char *errinfo,
				const void *private_info, size_t cb, void *user_data),
		void *user_data, cl_int *errcode_ret) {
	cl_context context = nullptr;
	cl_int errcode = CL_SUCCESS;

	/*
	 * Create context
	 */
    try {
        std::unique_ptr<dclicd::detail::ContextProperties> properties_;
        if (properties) properties_.reset(new dclicd::detail::ContextProperties(properties));

        context = new _cl_context(properties_.get(), device_type, pfn_notify,
                user_data);
    } catch (const dclicd::Error& err) {
        errcode = err.err();
    } catch (const std::bad_alloc& err) {
        errcode = CL_OUT_OF_HOST_MEMORY;
    }

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return context;
}

cl_int clRetainContext(cl_context context) {
    return clRetain(context);
}

cl_int clReleaseContext(cl_context context) {
	return clRelease(context);
}

cl_int clGetContextInfo(cl_context context, cl_context_info param_name,
		size_t param_value_size, void *param_value,
		size_t *param_value_size_ret) {
    return clGetInfo(context, param_name, param_value_size, param_value, param_value_size_ret);
}

/* Command Queue APIs */
cl_command_queue clCreateCommandQueue(cl_context context, cl_device_id device,
		cl_command_queue_properties properties, cl_int *errcode_ret) {
	cl_command_queue command_queue = nullptr;
	cl_int errcode = CL_SUCCESS;

	try {
		command_queue = new _cl_command_queue(context, device, properties);
	} catch (const dclicd::Error& err) {
		errcode = err.err();
	} catch (const std::bad_alloc&) {
		errcode = CL_OUT_OF_HOST_MEMORY;
	}

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return command_queue;
}

cl_int clRetainCommandQueue(cl_command_queue command_queue) {
	return clRetain(command_queue);
}

cl_int clReleaseCommandQueue(cl_command_queue command_queue) {
    return clRelease(command_queue);
}

cl_int clGetCommandQueueInfo(cl_command_queue command_queue,
		cl_command_queue_info param_name, size_t param_value_size,
		void *param_value, size_t *param_value_size_ret) {
    return clGetInfo(command_queue, param_name, param_value_size, param_value, param_value_size_ret);
}

#if defined(CL_USE_DEPRECATED_OPENCL_1_0_APIS) || (defined(CL_VERSION_1_0) && !defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
cl_int clSetCommandQueueProperty(
		cl_command_queue command_queue,
		cl_command_queue_properties properties,
		cl_bool enable,
		cl_command_queue_properties *old_properties) {
	/* clSetCommandQueueProperty is deprecated since OpenCL 1.1 and will not be
	 * implemented in dOpenCL */
    assert(!"clSetCommandQueueProperty not implemented");
	return CL_SUCCESS;
}
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_0_APIS)

/* Memory Object APIs */
cl_mem clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size,
		void *host_ptr, cl_int *errcode_ret) {
	cl_mem buffer = nullptr;
	cl_int errcode = CL_SUCCESS;

	try {
		buffer = new dclicd::Buffer(context, flags, size, host_ptr);
	} catch (const dclicd::Error& err) {
		errcode = err.err();
	} catch (const std::bad_alloc&) {
		errcode = CL_OUT_OF_HOST_MEMORY;
	}

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return buffer;
}

cl_mem clCreateSubBuffer(
        cl_mem buffer,
        cl_mem_flags flags,
        cl_buffer_create_type buffer_create_type,
        const void *buffer_create_info,
        cl_int *errcode_ret) {
    assert(!"clCreateSubBuffer not implemented");
    return nullptr;
}

cl_int clRetainMemObject(cl_mem memobj) {
	return clRetain(memobj);
}

cl_int clReleaseMemObject(cl_mem memobj) {
	return clRelease(memobj);
}

cl_int clGetSupportedImageFormats(
        cl_context context,
        cl_mem_flags flags,
        cl_mem_object_type image_type,
        cl_uint num_entries,
        cl_image_format *image_formats,
        cl_uint *num_image_formats) {
    if (!context) return CL_INVALID_CONTEXT;

    if (image_formats) {
        if (num_entries == 0) return CL_INVALID_VALUE;
//  The spec does not define behavior if image_formats and num_image_formats are NULL
//    } else {
//        if (!num_image_formats) return CL_INVALID_VALUE;
    }

    try {
        std::vector<cl_image_format> imageFormats_;

        context->getSupportedImageFormats(flags, image_type, imageFormats_);

        if (image_formats) {
            auto imageFormat = std::begin(imageFormats_);
			for (cl_uint i = 0; i < num_entries && imageFormat != std::end(imageFormats_); ++i) {
				*image_formats++ = *imageFormat++;
			}
        }
        if (num_image_formats) *num_image_formats = imageFormats_.size();
    } catch (const dclicd::Error& err) {
        return err.err();
    }

    return CL_SUCCESS;
}

cl_int clGetMemObjectInfo(cl_mem mem,
		cl_mem_info param_name, size_t param_value_size,
		void *param_value, size_t *param_value_size_ret) {
    return clGetInfo(mem, param_name, param_value_size, param_value, param_value_size_ret);
}

cl_int clSetMemObjectDestructorCallback(cl_mem memobj,
		void (CL_CALLBACK *pfn_notify)(cl_mem, void *),
		void *user_data) {
	if (memobj) {
		try {
			memobj->setDestructorCallback(pfn_notify, user_data);
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_MEM_OBJECT;
	}

	return CL_SUCCESS;
}

/* Sampler APIs */

/* Program Object APIs */
cl_program clCreateProgramWithSource(cl_context context, cl_uint count,
		const char **strings, const size_t *lengths, cl_int *errcode_ret) {
	cl_program program = nullptr;
	cl_int errcode = CL_SUCCESS;
	_cl_program::Sources sources;

	if (!strings) {
		if (errcode_ret) {
			*errcode_ret = CL_INVALID_VALUE;
		}
		return program;
	}

	/*
	 * Create sources from C strings
	 */
	if (lengths) {
		for (unsigned int i = 0; i < count; ++i) {
			if (!strings[i]) {
				if (errcode_ret) {
					*errcode_ret = CL_INVALID_VALUE;
				}
				return program;
			}

			if (lengths[i] == 0) {
				/* string is NULL terminated */
				sources.push_back(std::make_pair(strings[i], strlen(strings[i])));
			} else {
				sources.push_back(std::make_pair(strings[i], lengths[i]));
			}
		}
	} else {
		for (unsigned int i = 0; i < count; ++i) {
			if (!strings[i]) {
				if (errcode_ret) {
					*errcode_ret = CL_INVALID_VALUE;
				}
				return program;
			}

			sources.push_back(std::make_pair(strings[i], strlen(strings[i])));
		}
	}

	try {
		program = new _cl_program(context, sources);
	} catch (const dclicd::Error& err) {
		errcode = err.err();
	} catch (const std::bad_alloc&) {
		errcode = CL_OUT_OF_HOST_MEMORY;
	}

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return program;
}

cl_program clCreateProgramWithBinary(
        cl_context context,
        cl_uint num_devices,
        const cl_device_id *device_list,
        const size_t *lengths,
        const unsigned char **bytes,
        cl_int *binary_status,
        cl_int *errcode_ret) {
    cl_program program = nullptr;
    cl_int errcode = CL_SUCCESS;
    _cl_program::Binaries binaries;

    if (num_devices == 0 || !device_list ||
            !lengths ||
            !bytes) {
        if (errcode_ret) {
            *errcode_ret = CL_INVALID_VALUE;
        }
        return program;
    }

    /*
     * Create binaries from bytes
     */
    binaries.reserve(num_devices);
    for (unsigned int i = 0; i < num_devices; ++i) {
        if (!lengths[i] || !bytes[i]) {
            if (errcode_ret) {
                *errcode_ret = CL_INVALID_VALUE;
            }
            return program;
        }

        binaries.push_back(std::make_pair(bytes[i], lengths[i]));
    }

    try {
        std::vector<cl_int> binaryStatus;

        program = new _cl_program(context,
                std::vector<cl_device_id>(device_list, device_list + num_devices),
                binaries,
                (binary_status ? &binaryStatus : nullptr));

        /* IMPORTANT: binary status must also be returned if CL_INVALID_BINARY
         *            is raised */
        assert(binaryStatus.size() == num_devices);
        if (binary_status) {
            std::copy(std::begin(binaryStatus), std::end(binaryStatus), binary_status);
        }

        errcode = CL_SUCCESS;
    } catch (const dclicd::Error& err) {
        errcode = err.err();
    } catch (const std::bad_alloc&) {
        errcode = CL_OUT_OF_HOST_MEMORY;
    }

    if (errcode_ret) {
        *errcode_ret = errcode;
    }

    return program;
}

#if defined(CL_VERSION_1_2)
cl_program clCreateProgramWithBuiltInKernels(
        cl_context context,
        cl_uint num_devices,
        const cl_device_id *device_list,
        const char *kernel_names,
        cl_int *errcode_ret) {
    assert(!"clCreateProgramWithBuiltInKernels not implemented");
    return nullptr;
}
#endif // #if defined(CL_VERSION_1_2)

cl_int clRetainProgram(cl_program program) {
	return clRetain(program);
}

cl_int clReleaseProgram(cl_program program) {
	return clRelease(program);
}

cl_int clBuildProgram(cl_program program, cl_uint num_devices,
		const cl_device_id *device_list, const char *options,
		void(*pfn_notify)(cl_program, void *), void *user_data) {
    std::unique_ptr<std::vector<cl_device_id>> _devices;
	cl_int errcode = CL_SUCCESS;

	if (!program) return CL_INVALID_PROGRAM;

	if ((!device_list && num_devices > 0) ||
			(device_list && num_devices == 0)) {
		return CL_INVALID_VALUE;
	}

	/*
	 * Convert device list
	 */
	if (device_list) {
		_devices.reset(new std::vector<cl_device_id>(device_list,
				device_list + num_devices));
	}

	/*
	 * Build program
	 */
	try {
		// be generous and accept NULL pointer as options
		program->build(_devices.get(), options, pfn_notify, user_data);
	} catch (const dclicd::Error& err) {
		errcode = err.err();
	}

	return errcode;
}

#if defined(CL_VERSION_1_2)
cl_int clCompileProgram(
        cl_program program,
        cl_uint num_devices,
        const cl_device_id * device_list,
        const char * options,
        cl_uint num_input_headers,
        const cl_program * input_headers,
        const char ** header_include_names,
        void(CL_CALLBACK * pfn_notify)(cl_program, void *),
        void * user_data) {
    std::unique_ptr<std::vector<cl_device_id>> _devices;
    std::unique_ptr<_cl_program::Headers> _inputHeaders;
    cl_int errcode = CL_SUCCESS;

    if (!program) return CL_INVALID_PROGRAM;

    if ((!device_list && num_devices > 0) ||
            (device_list && num_devices == 0)) {
        return CL_INVALID_VALUE;
    }

    if ((!input_headers && !header_include_names && num_input_headers > 0) ||
            ((input_headers || header_include_names) && num_input_headers == 0)) {
        return CL_INVALID_VALUE;
    }

    /*
     * Convert device list
     */
    if (device_list) {
        _devices.reset(new std::vector<cl_device_id>(device_list,
                device_list + num_devices));
    }

    /*
     * Convert input header list
     */
    if (input_headers) {
        _inputHeaders.reset(new _cl_program::Headers());
        for (unsigned int i = 0; i < num_input_headers; ++i) {
            _inputHeaders->insert(std::make_pair(header_include_names[i], input_headers[i]));
        }
    }

    /*
     * Compile program
     */
    try {
        // be generous and accept nullptr as options
        program->compile(_devices.get(), options, _inputHeaders.get(), pfn_notify, user_data);
    } catch (const dclicd::Error& err) {
        errcode = err.err();
    }

    return errcode;
}

cl_program clLinkProgram(
        cl_context context,
        cl_uint num_devices,
        const cl_device_id * device_list,
        const char * options,
        cl_uint num_input_programs,
        const cl_program * input_programs,
        void(CL_CALLBACK * pfn_notify)(cl_program, void *),
        void * user_data,
        cl_int * errcode_ret) {
    assert(!"clLinkProgram not implemented");
    return nullptr;
}
#endif // #if defined(CL_VERSION_1_2)

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
cl_int clUnloadCompiler(void) {
	/* clUnloadCompiler is deprecated since OpenCL 1.2 and is implemented as a
	 * no-operation in dOpenCL */
	return CL_SUCCESS;
}
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

cl_int clUnloadPlatformCompiler(cl_platform_id platform) {
	if (platform) {
		platform->unloadCompiler();
	} else {
		return CL_INVALID_PLATFORM;
	}

	return CL_SUCCESS;
}

cl_int clGetProgramInfo(cl_program program, cl_program_info param_name,
		size_t param_value_size, void *param_value,
		size_t *param_value_size_ret) {
    return clGetInfo(program, param_name, param_value_size, param_value, param_value_size_ret);
}

cl_int clGetProgramBuildInfo(cl_program program, cl_device_id device,
		cl_program_build_info param_name, size_t param_value_size,
		void *param_value, size_t *param_value_size_ret) {
	if (program) {
		try {
			program->getBuildInfo(device, param_name, param_value_size,
					param_value, param_value_size_ret);
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_PROGRAM;
	}

	return CL_SUCCESS;
}

/* Kernel Object APIs */
cl_kernel clCreateKernel(cl_program program, const char *kernel_name,
		cl_int *errcode_ret) {
	cl_kernel kernel = nullptr;
	cl_int errcode = CL_SUCCESS;

	if (!kernel_name) {
		if (errcode_ret) {
			*errcode_ret = CL_INVALID_VALUE;
		}
		return kernel;
	}

	try {
		kernel = new _cl_kernel(program, kernel_name);
	} catch (const dclicd::Error& err) {
		errcode = err.err();
	} catch (const std::bad_alloc&) {
		errcode = CL_OUT_OF_HOST_MEMORY;
	}

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return kernel;
}

cl_int clCreateKernelsInProgram(cl_program program, cl_uint num_kernels,
		cl_kernel *kernels, cl_uint *num_kernels_ret) {
	if (!program) return CL_INVALID_PROGRAM;
	if (!kernels && (num_kernels > 0)) return CL_INVALID_VALUE;

	try {
	    cl_uint num_kernels_in_program;

        /* Query number of kernels from program */
        program->getInfo(CL_PROGRAM_NUM_KERNELS, sizeof(num_kernels_in_program),
                &num_kernels_in_program, nullptr);

		if (kernels) {
            std::vector<cl_kernel> kernels_;

		    if (num_kernels < num_kernels_in_program) return CL_INVALID_VALUE;

			_cl_kernel::createKernelsInProgram(program, kernels_);
			assert(num_kernels >= kernels_.size());

			/* Copy kernels */
			std::copy(std::begin(kernels_), std::end(kernels_), kernels);
		}

        if (num_kernels_ret) {
            *num_kernels_ret = num_kernels_in_program;
        }
	} catch (const dclicd::Error& err) {
		return err.err();
	} catch (const std::bad_alloc&) {
		return CL_OUT_OF_HOST_MEMORY;
	}

	return CL_SUCCESS;
}

cl_int clRetainKernel(cl_kernel kernel) {
	return clRetain(kernel);
}

cl_int clReleaseKernel(cl_kernel kernel) {
	return clRelease(kernel);
}

cl_int clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size,
		const void *arg_value) {
	if (kernel) {
		try {
			kernel->setArgument(arg_index, arg_size, arg_value);
		} catch (const dclicd::Error& err) {
			return err.err();
		} catch (const std::bad_alloc&) {
			return CL_OUT_OF_HOST_MEMORY;
		}
	} else {
		return CL_INVALID_KERNEL;
	}

	return CL_SUCCESS;
}

cl_int clGetKernelInfo(cl_kernel kernel, cl_kernel_info param_name,
		size_t param_value_size, void *param_value,
		size_t *param_value_size_ret) {
    return clGetInfo(kernel, param_name, param_value_size, param_value, param_value_size_ret);
}

#if defined(CL_VERSION_1_2)
cl_int clGetKernelArgInfo(
        cl_kernel kernel,
        cl_uint arg_indx,
        cl_kernel_arg_info param_name,
        size_t param_value_size,
        void * param_value,
        size_t * param_value_size_ret) {
    if (kernel) {
        try {
            kernel->getArgInfo(arg_indx, param_name, param_value_size,
                    param_value, param_value_size_ret);
        } catch (const dclicd::Error& err) {
            return err.err();
        }
    } else {
        return CL_INVALID_KERNEL;
    }

    return CL_SUCCESS;
}
#endif // #if defined(CL_VERSION_1_2)

cl_int clGetKernelWorkGroupInfo(
        cl_kernel kernel,
        cl_device_id device,
        cl_kernel_work_group_info param_name,
        size_t param_value_size,
        void * param_value,
        size_t * param_value_size_ret) {
    if (kernel) {
        try {
            kernel->getWorkGroupInfo(device, param_name, param_value_size,
                    param_value, param_value_size_ret);
        } catch (const dclicd::Error& err) {
            return err.err();
        }
    } else {
        return CL_INVALID_KERNEL;
    }

    return CL_SUCCESS;
}

/* Event Object APIs */
cl_int clWaitForEvents(cl_uint num_events, const cl_event *event_list) {
	if (num_events == 0 || !event_list) {
		return CL_INVALID_VALUE;
	}

	try {
		_cl_event::waitForEvents(std::vector<cl_event>(event_list,
				event_list + num_events));
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

cl_int clGetEventInfo(cl_event event, cl_profiling_info param_name,
        size_t param_value_size, void *param_value,
        size_t *param_value_size_ret) {
    return clGetInfo(event, param_name, param_value_size, param_value, param_value_size_ret);
}

cl_int clRetainEvent(cl_event event) {
	return clRetain(event);
}

cl_int clReleaseEvent(cl_event event) {
	return clRelease(event);
}

cl_event clCreateUserEvent(cl_context context, cl_int *errcode_ret) {
	cl_event event = nullptr;
	cl_int errcode = CL_SUCCESS;

	try {
		event = new dclicd::UserEvent(context);
	} catch (const dclicd::Error& err) {
		errcode = err.err();
	} catch (const std::bad_alloc&) {
		errcode = CL_OUT_OF_HOST_MEMORY;
	}

	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return event;
}

cl_int clSetUserEventStatus(cl_event event, cl_int execution_status) {
	dclicd::UserEvent *user_event = dynamic_cast<dclicd::UserEvent *>(event);

	if (user_event) {
		try {
			user_event->setStatus(execution_status);
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_EVENT;
	}

	return CL_SUCCESS;
}

cl_int clSetEventCallback(
		cl_event event,
		cl_int command_exec_callback_type,
		void (CL_CALLBACK *pfn_event_notify)(
				cl_event event,
				cl_int event_command_exec_status,
				void *user_data),
		void * user_data) {
	if (event) {
		try {
			event->setCallback(
					command_exec_callback_type, pfn_event_notify, user_data);
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_EVENT;
	}

	return CL_SUCCESS;
}

/* Profiling APIs */
cl_int clGetEventProfilingInfo(cl_event event, cl_profiling_info param_name,
		size_t param_value_size, void *param_value,
		size_t *param_value_size_ret) {
	if (event) {
		try {
			event->getProfilingInfo(param_name, param_value_size, param_value,
					param_value_size_ret);
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_EVENT;
	}

	return CL_SUCCESS;
}

/* Flush and Finish APIs */
cl_int clFlush(cl_command_queue command_queue) {
	if (command_queue) {
		try {
			command_queue->flush();
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_COMMAND_QUEUE;
	}

	return CL_SUCCESS;
}

cl_int clFinish(cl_command_queue command_queue) {
	if (command_queue) {
		try {
			command_queue->finish();
		} catch (const dclicd::Error& err) {
			return err.err();
		}
	} else {
		return CL_INVALID_COMMAND_QUEUE;
	}

	return CL_SUCCESS;
}

/* Enqueued Commands APIs */
cl_int clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem mem,
		cl_bool blocking_read, size_t offset, size_t cb, void *ptr,
		cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
		cl_event *event) {
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_VALUE;
	}

	try {
		command_queue->enqueueRead(
		        dynamic_cast<dclicd::Buffer *>(mem),
		        blocking_read, offset, cb,
				ptr, std::vector<cl_event>(event_wait_list,
						event_wait_list + num_events_in_wait_list), event);
	} catch (const dclicd::Error& err) {
		return err.err();
	} catch (const std::bad_alloc&) {
		return CL_OUT_OF_HOST_MEMORY;
	}

	return CL_SUCCESS;
}

cl_int clEnqueueReadBufferRect(
        cl_command_queue command_queue,
        cl_mem buffer,
        cl_bool blocking_read,
        const size_t *buffer_origin,
        const size_t *host_origin,
        const size_t *region,
        size_t buffer_row_pitch,
        size_t buffer_slice_pitch,
        size_t host_row_pitch,
        size_t host_slice_pitch,
        void *ptr,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event) {
    assert(!"clEnqueueReadBufferRect not implemented");
    return CL_SUCCESS;
}

cl_int clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem mem,
		cl_bool blocking_write, size_t offset, size_t cb, const void * ptr,
		cl_uint num_events_in_wait_list, const cl_event * event_wait_list,
		cl_event * event) {
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_VALUE;
	}

	try {
		command_queue->enqueueWrite(
		        dynamic_cast<dclicd::Buffer *>(mem),
		        blocking_write, offset, cb,
				ptr, std::vector<cl_event>(event_wait_list,
						event_wait_list + num_events_in_wait_list), event);
	} catch (const dclicd::Error& err) {
		return err.err();
	} catch (const std::bad_alloc&) {
		/* TODO Catch bad_alloc within _cl_command_queue::enqueueWriteBuffer */
		return CL_OUT_OF_HOST_MEMORY;
	}

	return CL_SUCCESS;
}

cl_int clEnqueueWriteBufferRect(
        cl_command_queue command_queue,
        cl_mem buffer,
        cl_bool blocking_write,
        const size_t *buffer_origin,
        const size_t *host_origin,
        const size_t *region,
        size_t buffer_row_pitch,
        size_t buffer_slice_pitch,
        size_t host_row_pitch,
        size_t host_slice_pitch,
        const void *ptr,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event) {
    assert(!"clEnqueueWriteBufferRect not implemented");
    return CL_SUCCESS;
}

#if defined(CL_VERSION_1_2)
cl_int clEnqueueFillBuffer(
        cl_command_queue command_queue,
        cl_mem buffer,
        const void *pattern,
        size_t pattern_size,
        size_t offset,
        size_t size,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event) {
    assert(!"clEnqueueFillBuffer not implemented");
    return CL_SUCCESS;
}
#endif // #if defined(CL_VERSION_1_2)

cl_int clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer,
		cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t cb,
		cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
		cl_event *event) {
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_VALUE;
	}

	try {
		command_queue->enqueueCopy(
		        dynamic_cast<dclicd::Buffer *>(src_buffer),
		        dynamic_cast<dclicd::Buffer *>(dst_buffer),
		        src_offset, dst_offset, cb,
				std::vector<cl_event>(event_wait_list, event_wait_list
						+ num_events_in_wait_list), event);
	} catch (const dclicd::Error& err) {
		return err.err();
	} catch (const std::bad_alloc&) {
		/* TODO Catch bad_alloc within _cl_command_queue::enqueueCopy */
		return CL_OUT_OF_HOST_MEMORY;
	}

	return CL_SUCCESS;
}

cl_int clEnqueueCopyBufferRect(
        cl_command_queue command_queue,
        cl_mem src_buffer,
        cl_mem dst_buffer,
        const size_t *src_origin,
        const size_t *dst_origin,
        const size_t *region,
        size_t src_row_pitch,
        size_t src_slice_pitch,
        size_t dst_row_pitch,
        size_t dst_slice_pitch,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event) {
    assert(!"clEnqueueCopyBufferRect not implemented");
    return CL_SUCCESS;
}

void * clEnqueueMapBuffer(cl_command_queue command_queue, cl_mem mem,
		cl_bool blocking_map, cl_map_flags map_flags, size_t offset, size_t cb,
		cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
		cl_event *event, cl_int *errcode_ret) {
	void *ptr = nullptr;
	cl_int errcode = CL_SUCCESS;

	if (command_queue) {
		if ((num_events_in_wait_list > 0 && !event_wait_list)
				|| (num_events_in_wait_list == 0 && event_wait_list)) {
			errcode = CL_INVALID_VALUE;
		} else {
            try {
                ptr = command_queue->enqueueMap(
                        dynamic_cast<dclicd::Buffer *>(mem),
                        blocking_map, map_flags, offset, cb,
                        std::vector<cl_event>(event_wait_list, event_wait_list + num_events_in_wait_list),
                        event);
            } catch (const dclicd::Error& err) {
                errcode = err.err();
            }
		}
	} else {
		errcode = CL_INVALID_COMMAND_QUEUE;
	}

	/* return error code */
	if (errcode_ret) {
		*errcode_ret = errcode;
	}

	return ptr;
}

cl_int clEnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj,
		void *mapped_ptr, cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list, cl_event *event) {
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_VALUE;
	}

	try {
		command_queue->enqueueUnmap(memobj, mapped_ptr,
				std::vector<cl_event>(event_wait_list, event_wait_list + num_events_in_wait_list),
				event);
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

#if defined(CL_VERSION_1_2)
cl_int clEnqueueMigrateMemObjects(
        cl_command_queue command_queue,
        cl_uint num_mem_objects,
        const cl_mem *mem_objects,
        cl_mem_migration_flags flags,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event) {
    /*
     * Validate parameters
     */
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;

    if (num_mem_objects == 0 || !mem_objects) return CL_INVALID_VALUE;
    if ((num_events_in_wait_list > 0 && !event_wait_list)
            || (num_events_in_wait_list == 0 && event_wait_list)) {
        return CL_INVALID_VALUE;
    }

    try {
        command_queue->enqueueMigrateMemObjects(
                std::vector<cl_mem>(mem_objects, mem_objects + num_mem_objects),
                flags,
                std::vector<cl_event>(event_wait_list, event_wait_list + num_events_in_wait_list),
                event);
    } catch (const dclicd::Error& err) {
        return err.err();
    }

    return CL_SUCCESS;
}
#endif // #if defined(CL_VERSION_1_2)

cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel kernel,
		cl_uint work_dim, const size_t * global_work_offset,
		const size_t * global_work_size, const size_t * local_work_size,
		cl_uint num_events_in_wait_list, const cl_event * event_wait_list,
		cl_event * event) {
	/*
	 * Validate parameters
	 */
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;

	if (work_dim < 1 || work_dim > 3) {
		return CL_INVALID_WORK_DIMENSION;
	}
	if (!global_work_size) return CL_INVALID_GLOBAL_WORK_SIZE;

	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_VALUE;
	}

	try {
		std::vector<size_t> offset;
		std::vector<size_t> global(global_work_size, global_work_size + work_dim);
		std::vector<size_t> local;

		/* Convert global work offset and local work size */
		if (global_work_offset) {
			offset.assign(global_work_offset, global_work_offset + work_dim);
		}
		if (local_work_size) {
			local.assign(local_work_size, local_work_size + work_dim);
		}

		command_queue->enqueueNDRangeKernel(kernel, offset, global, local,
				std::vector<cl_event>(event_wait_list, event_wait_list + num_events_in_wait_list),
				event);
	} catch (const dclicd::Error& err) {
		return err.err();
	} catch (const std::bad_alloc&) {
		return CL_OUT_OF_HOST_MEMORY;
	}

	return CL_SUCCESS;
}

cl_int clEnqueueTask(
        cl_command_queue command_queue,
        cl_kernel kernel,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event) {
    /*
     * Validate parameters
     */
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;

    if ((num_events_in_wait_list > 0 && !event_wait_list)
            || (num_events_in_wait_list == 0 && event_wait_list)) {
        return CL_INVALID_VALUE;
    }

    try {
        command_queue->enqueueTask(kernel,
                std::vector<cl_event>(event_wait_list, event_wait_list + num_events_in_wait_list),
                event);
    } catch (const dclicd::Error& err) {
        return err.err();
    } catch (const std::bad_alloc&) {
        return CL_OUT_OF_HOST_MEMORY;
    }

    return CL_SUCCESS;
}

cl_int clEnqueueNativeKernel(
        cl_command_queue command_queue,
        void (*user_func)(void *),
        void *args,
        size_t cb_args,
        cl_uint num_mem_objects,
        const cl_mem *mem_list,
        const void **args_mem_loc ,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event) {
    /*
     * Validate parameters
     */
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;

    if (!user_func) return CL_INVALID_VALUE;
    if ((!args && (cb_args == 0 || num_mem_objects == 0))
            || (args && cb_args == 0)) {
        return CL_INVALID_VALUE;
    }
    if ((num_mem_objects > 0 && (!mem_list || !args_mem_loc))
            || (num_mem_objects == 0 && (mem_list || args_mem_loc))) {
        return CL_INVALID_VALUE;
    }
    if ((num_events_in_wait_list > 0 && !event_wait_list)
            || (num_events_in_wait_list == 0 && event_wait_list)) {
        return CL_INVALID_VALUE;
    }

    /*
     * Native kernels cannot be executed by remote events. As all devices in
     * dOpenCL are remote, all command-queues are associated with a remote
     * device such that enqueuing a native kernel always is an invalid
     * operation.
     */
    return CL_INVALID_OPERATION;
}

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
cl_int clEnqueueMarker(cl_command_queue command_queue, cl_event *event) {
	/* Implement clEnqueueMarker using OpenCL 1.2 API */

	/* Event must not be NULL in OpenCL 1.1 API, but may be NULL in OpenCL 1.2
	 * clEnqueueMarkerWithWaitList API */
	if (!event) return CL_INVALID_VALUE;
	
	return clEnqueueMarkerWithWaitList(command_queue, 0, nullptr, event);
}

cl_int clEnqueueWaitForEvents(cl_command_queue command_queue,
		cl_uint num_events, const cl_event *event_list) {
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
	if (num_events == 0 || !event_list) return CL_INVALID_VALUE;

	try {
		command_queue->enqueueWaitForEvents(
				std::vector<cl_event>(event_list, event_list + num_events));
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

cl_int clEnqueueBarrier(cl_command_queue command_queue) {
	/* Implement clEnqueueBarrier using OpenCL 1.2 API */
	return clEnqueueBarrierWithWaitList(command_queue, 0, nullptr, nullptr);
}
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

cl_int clEnqueueMarkerWithWaitList(cl_command_queue command_queue,
		cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
		cl_event *event) {
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;

	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_EVENT_WAIT_LIST;
	}

	try {
		command_queue->enqueueMarker(
				std::vector<cl_event>(event_wait_list,
						event_wait_list + num_events_in_wait_list),
				event);
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

cl_int clEnqueueBarrierWithWaitList(cl_command_queue command_queue,
		cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
		cl_event *event) {
	if (!command_queue) return CL_INVALID_COMMAND_QUEUE;

	if ((num_events_in_wait_list > 0 && !event_wait_list)
			|| (num_events_in_wait_list == 0 && event_wait_list)) {
		return CL_INVALID_EVENT_WAIT_LIST;
	}

	try {
		command_queue->enqueueBarrier(
				std::vector<cl_event>(event_wait_list,
						event_wait_list + num_events_in_wait_list),
				event);
	} catch (const dclicd::Error& err) {
		return err.err();
	}

	return CL_SUCCESS;
}

/******************************************************************************/

#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) || (defined(CL_VERSION_1_1) && !defined(CL_VERSION_1_2))
/**
 * @brief Returns the address of the extension function named by \c func_name.
 *
 * WARNING:
 * This function is deprecated since OpenCL 1.2. It has been replaced by
 * clGetExtensionFunctionAddressForPlatform.
 *
 * @param[in]  func_name    Name of an extension function
 * @return the address of the extension function named by func_name.
 */
void * clGetExtensionFunctionAddress(const char *func_name) {
    /* Implement clGetExtensionFunctionAddress using OpenCL 1.2 API */

    /* Get extension function address for default platform dOpenCL */
    return clGetExtensionFunctionAddressForPlatform(
            _cl_platform_id::dOpenCL(), func_name);
}
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

/**
 * @brief Returns the address of the extension function named by \c func_name for a given platform.
 *
 * Use option '-Bsymbolic-functions' to make the linker (ld) bind global
 * function symbols to function symbols within a shared library, if any.
 * Otherwise the function pointers returned by this function may not refer
 * to the dOpenCL implementations of that functions, as these may be
 * overwritten, i.e. also implemented, by an ICD loader (or any other
 * program) that dynamically loads dOpenCL.
 *
 * @param[in]  platform     Platform to query the extension function for
 * @param[in]  func_name    Name of an extension function
 * @return the address of the extension function named by \c func_name.
 */
void * clGetExtensionFunctionAddressForPlatform(cl_platform_id platform,
		const char *func_name) {
    if (!strcmp(func_name, "clCreateComputeNodeWWU")) {
        return reinterpret_cast<void *> (&clCreateComputeNodeWWU);
    }
    if (!strcmp(func_name, "clRetainComputeNodeWWU")) {
        return reinterpret_cast<void *> (&clRetainComputeNodeWWU);
    }
    if (!strcmp(func_name, "clReleaseComputeNodeWWU")) {
        return reinterpret_cast<void *> (&clReleaseComputeNodeWWU);
    }
    if (!strcmp(func_name, "clGetComputeNodesWWU")) {
        return reinterpret_cast<void *> (&clGetComputeNodesWWU);
    }
    if (!strcmp(func_name, "clGetComputeNodeInfoWWU")) {
        return reinterpret_cast<void *> (&clGetComputeNodeInfoWWU);
    }

    if (!strcmp(func_name, "clCreateContextFromComputeNodesWWU")) {
        return reinterpret_cast<void *> (&clCreateContextFromComputeNodesWWU);
    }

    if (!strcmp(func_name, "clEnqueueBroadcastBufferWWU")) {
        return reinterpret_cast<void *> (&clEnqueueBroadcastBufferWWU);
    }
    if (!strcmp(func_name, "clEnqueueReduceBufferWWU")) {
        return reinterpret_cast<void *> (&clEnqueueReduceBufferWWU);
    }

    return nullptr;
}
