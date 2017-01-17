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
 * @file cl_khr_icd.cpp
 *
 * @date 2011-02-22
 * @author Philipp Kegel
 */

#include "cl_khr_icd.h"

#include <cstdlib>

/* FIXME Link with option '-Bsymbolic-functions' */
/* TODO Check if there are other side-effects of '-Bsymbolic-functions' */
/**
 * ICD function pointer dispatch table
 *
 * Use option '-Bsymbolic-functions' to make the linker (ld) bind global
 * function symbols to function symbols within a shared library, if any.
 * (Remember to prefix linker options with '-Wl,' when the linker is called
 * indirectly via gcc, i.e. use '-Wl,-Bsymbolic-functions')
 * Thus the global functions symbols within the shared library names cannot be
 * overwritten by a program linked against this library.
 * For example, the function pointer within the ICD function pointer dispatch
 * table will be bound to the functions within this library rather than to the
 * functions of an ICD loader, which provides the same global function symbols.
 */
struct _cl_icd_dispatch dispatch = {
/******************************************************************************
 * OpenCL 1.0 APIs
 ******************************************************************************/

        /* Platform APIs */
        NULL, /* clGetPlatformIDs */
        &clGetPlatformInfo,

        /* Device APIs */
        &clGetDeviceIDs,
        NULL, /* clGetDeviceInfo */

        /* Context APIs */
        NULL, /* clCreateContext */
        NULL, /* clCreateContextFromType */
        &clRetainContext,
        &clReleaseContext,
        &clGetContextInfo,

        /* Command Queue APIs */
        &clCreateCommandQueue,
        &clRetainCommandQueue,
        &clReleaseCommandQueue,
        &clGetCommandQueueInfo,
#ifdef CL_USE_DEPRECATED_OPENCL_1_0_APIS
        &clSetCommandQueueProperty,
#else
        NULL, /* clSetCommandQueueProperty (deprecated) */
#endif /* CL_USE_DEPRECATED_OPENCL_1_0_APIS */

        /* Memory Object APIs */
        &clCreateBuffer,
#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
        NULL, //&clCreateImage2D,
        NULL, //&clCreateImage3D,
#else
        NULL /* clCreateImage2D (deprecated) */,
        NULL /* clCreateImage3D (deprecated) */,
#endif /* CL_USE_DEPRECATED_OPENCL_1_1_APIS */
        &clRetainMemObject,
        &clReleaseMemObject,
        &clGetSupportedImageFormats,
        &clGetMemObjectInfo,
        NULL, //&clGetImageInfo,

        /* Sampler APIs  */
        NULL, //&clCreateSampler,
        NULL, //&clRetainSampler,
        NULL, //&clReleaseSampler,
        NULL, //&clGetSamplerInfo,

        /* Program Object APIs  */
        &clCreateProgramWithSource,
        &clCreateProgramWithBinary,
        &clRetainProgram,
        &clReleaseProgram,
        &clBuildProgram,
        NULL /* clUnloadCompiler (not supported by ICD loader, deprecated) */,
        &clGetProgramInfo,
        &clGetProgramBuildInfo,

        /* Kernel Object APIs */
        &clCreateKernel,
        &clCreateKernelsInProgram,
        &clRetainKernel,
        &clReleaseKernel,
        &clSetKernelArg,
        &clGetKernelInfo,
        &clGetKernelWorkGroupInfo,

        /* Event Object APIs  */
        &clWaitForEvents,
        &clGetEventInfo,
        &clRetainEvent,
        &clReleaseEvent,

        /* Profiling APIs  */
        &clGetEventProfilingInfo,

        /* Flush and Finish APIs */
        &clFlush,
        &clFinish,

        /* Enqueued Commands APIs */
        &clEnqueueReadBuffer,
        &clEnqueueWriteBuffer,
        &clEnqueueCopyBuffer,
        NULL, //&clEnqueueReadImage,
        NULL, //&clEnqueueWriteImage,
        NULL, //&clEnqueueCopyImage,
        NULL, //&clEnqueueCopyImageToBuffer,
        NULL, //&clEnqueueCopyBufferToImage,
        &clEnqueueMapBuffer,
        NULL, //&clEnqueueMapImage,
        &clEnqueueUnmapMemObject,
        &clEnqueueNDRangeKernel,
        &clEnqueueTask,
        &clEnqueueNativeKernel,
#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
        &clEnqueueMarker,
        &clEnqueueWaitForEvents,
        &clEnqueueBarrier,
#else
        NULL /* clEnqueueMarker (deprecated) */,
        NULL /* clEnqueueWaitForEvents (deprecated) */,
        NULL /* clEnqueueBarrier (deprecated) */,
#endif /* CL_USE_DEPRECATED_OPENCL_1_1_APIS */

        /* Extension function access */
#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
        &clGetExtensionFunctionAddress,
#else
        NULL, /* clGetExtensionFunctionAddress (deprecated) */
#endif /* CL_USE_DEPRECATED_OPENCL_1_1_APIS */

        /* OpenCL/OpenGL Sharing APIs */
        NULL, //&clCreateFromGLBuffer,
#ifdef CL_USE_DEPRECATED_OPENCL_1_1_APIS
        NULL, //&clCreateFromGLTexture2D,
        NULL, //&clCreateFromGLTexture3D,
#else
        NULL, /* clCreateFromGLTexture2D (deprecated) */
        NULL, /* clCreateFromGLTexture3D (deprecated) */
#endif /* CL_USE_DEPRECATED_OPENCL_1_1_APIS */
        NULL, //&clCreateFromGLRenderbuffer,
        NULL, //&clGetGLObjectInfo,
        NULL, //&clGetGLTextureInfo,
        NULL, //&clEnqueueAcquireGLObjects,
        NULL, //&clEnqueueReleaseGLObjects,

        /* OpenCL Events From OpenGL Syncs */
        NULL, //&clGetGLContextInfoKHR,

        /* Sharing With Direct3D 10 */
        NULL /* clGetDeviceIDsFromD3D10KHR */,
        NULL /* clCreateFromD3D10BufferKHR */,
        NULL /* clCreateFromD3D10Texture2DKHR */,
        NULL /* clCreateFromD3D10Texture3DKHR */,
        NULL /* clEnqueueAcquireD3D10ObjectsKHR */,
        NULL /* clEnqueueReleaseD3D10ObjectsKHR */,
#if defined(CL_VERSION_1_1)

/******************************************************************************
 * OpenCL 1.1 APIs
 ******************************************************************************/

        &clSetEventCallback,

        /* Memory Object APIs */
        &clCreateSubBuffer,
        &clSetMemObjectDestructorCallback,

        /* Event Object APIs  */
        &clCreateUserEvent,
        &clSetUserEventStatus,

        /* Enqueued Commands APIs */
        &clEnqueueReadBufferRect,
        &clEnqueueWriteBufferRect,
        &clEnqueueCopyBufferRect,
#endif // #if defined(CL_VERSION_1_1)

#if defined(CL_VERSION_1_2)
/******************************************************************************
 * OpenCL 1.2 APIs
 ******************************************************************************/

        /* Device APIs */
        &clCreateSubDevices,
        &clRetainDevice,
        &clReleaseDevice,

        /* Memory Object APIs */
        NULL, //&clCreateImage,

        /* Program Object APIs  */
        &clCreateProgramWithBuiltInKernels,
        &clCompileProgram,
        &clLinkProgram,
        &clUnloadPlatformCompiler,

        /* Kernel Object APIs */
        &clGetKernelArgInfo,

        /* Enqueued Commands APIs */
        &clEnqueueFillBuffer,
        NULL, //&clEnqueueFillImage,
        &clEnqueueMigrateMemObjects,
        &clEnqueueMarkerWithWaitList,
        &clEnqueueBarrierWithWaitList,
        NULL, //&clSetPrintfCallback,

        /* Extension function access */
        &clGetExtensionFunctionAddressForPlatform,

        /* OpenCL/OpenGL Sharing APIs */
        NULL, //&clCreateFromGLTexture,
#endif // #if defined(CL_VERSION_1_2)
};
