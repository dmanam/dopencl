dOpenCL
=======

(last update: 2014-12-22, Philipp Kegel)

dOpenCL (distributed Open Compute Language) is an implementation of the OpenCL
API for distributed systems created by Philipp Kegel with the help of Michel
Steuwer, Sergei Gorlatch, and several students.

References:
* Kegel, P., Steuwer, M., Gorlatch, S., 2012. dOpenCL: Towards a Unified
  Programming Approach for Distributed Heterogeneous Multi-/Many-Core Systems.


--------
Contents
--------

1. System requirements
2. Getting started
3. Project structure
4. Known issues
5. Contact


-------------------
System requirements
-------------------

See 'INSTALL_Linux.txt' for system requirements and installation instructions.

dOpenCL has been successfully tested on the following systems:

* Ubuntu 12.04
* CentOS 5.6, 5.9
* Scientific Linux 5.6


---------------
Getting started
---------------

Before running the dOpenCL daemon or your OpenCL application, you have to ensure
that dOpenCL is properly installed (see 'INSTALL_Linux.txt').

Setting up your environment (optional)
--------------------------------------

Unless dOpenCL has been installed into your default system directories, you have
you have to update your environment's PATH and LD_LIBRARY_PATH as follows (note
the use of the DCL_HOME environment variable):

   export PATH=$PATH:$DCL_HOME/daemon
   export LD_LIBRARY_PATH=$DCL_HOME/dclgcf:$DCL_HOME/icdpp:$LD_LIBRARY_PATH

Starting and stopping the dOpenCL daemon
----------------------------------------

In order to start the dOpenCL daemon you need to have an OpenCL 1.1+ compliant
OpenCL implementation installed on your system.

The dOpenCL daemon has to be started on all nodes that should be accessible via
dOpenCL. Currently, the daemon does not become a daemon process but blocks the
terminal.

  dcld [-p<platform name>] <hostname>[:<port>]

Hint: Ubuntu resolves all hostnames of the local system to the address of the
loopback interface. In order to bind the dOpenCL daemon to another interface,
the interface's IP address has to be provided rather than the interface's host
name.

The -p option selects the native OpenCL platform that should be used by the
dOpenCL daemon. The specified platform name must be part of the platform's full
name. If no platform is specified, the system's OpenCL platform is used.

The daemon is stopped by sending it a SIGINT (press Strg+C) or SIGTERM (kill)
signal.

Running an OpenCL application with dOpenCL
------------------------------------------

As dOpenCL *is* an OpenCL implementation, existing applications do not have to
be compiled or linked anew in order to use dOpenCL. However, the following
preparations are required to connect the host and daemons an to make the
application use the dOpenCL ICD.

1. Create a node file

   Create a file named 'dcl.nodes' *in your application's working directory.*
   Add a line with a host name and optional port number (in the format
   <hostname>[:<port>]) for each node of your network, on which a dOpenCL
   daemon is running. For example:

     echo localhost >> dcl.nodes

   You can also create a global node file anywhere in your systems and export
   the DCL_NODE_FILE environment variable. If DCL_NODE_FILE is defined and not
   empty, its value is taken as the location of the application's node file.
   The 'dcl.nodes' file in the application's working directory will be ignored
   in this case.

2. Run the application

   If you have replaced the system's ICD by dOpenCL's ICD, you may run your
   application as usual.

   Otherwise, you have to use the LD_PRELOAD environment variable in order to
   preload the dOpenCL ICD before running your OpenCL application:

     LD_PRELOAD=libdOpenCL.so <application binary> [<arguments>]

   To avoid explicitly setting this variable each time you run your application,
   you may export LD_PRELOAD to your environment:

     export LD_PRELOAD="$LD_PRELOAD libdOpenCL.so"

   We recommend to create a start script for you application which you use to
   export LD_PRELOAD (and LD_LIBRARY_PATH as described in 'Setting up your
   environment') before starting you application.

Controlling log output
----------------------

dOpenCL creates log files for debugging purposes. In the daemon's working
directory, a file named 'dcl_<host name>.log' is created, where <host name> will
be the host name you selected when starting the damon. In the application's
working directory, a file 'dcl_host.log' will be created.

Currently, logging cannot be switched off, but the amount of log messages can be
controlled by setting the log level in the DCL_LOG_LEVEL environment variable.
The following settings are eligible:

  ERROR    only log error messages
  WARNING  log warnings
  INFO     log info messages  (default for release build)
  DEBUG    log debug messages (default for debug build)
  VERBOSE  log everything

If no log level is specified, the default log level is selected.

Note that the log files are deleted each time the daemon or application is
restarted.


-----------------
Project structure
-----------------

/
+- dcl		                dOpenCL API definitions
|  +- doc			Doxygen documentation of dOpenCL API
|  +- include
|  |  +- CL                     OpenCL API extension
|  |  |  cl_wwu_collective.h    collective operations
|  |  |  cl_wwu_dcl.h           distributed OpenCL
|  |  +- dcl                    dOpenCL API headers
|  |        ComputeNode.h
|  |        CommunicationManager.h
|  |        Device.h
|  |        Host.h
|  |        ...
+- dclasio                      dOpenCL API implementation using Boost.Asio
+- icdpp                        ICD implementation (C++, Boost.Asio only)
|                               implements OpenCL (including API extension) using dOpenCL API
+- daemon                       dOpenCL daemon
+- test                         Test suite (based on Boost Test Library, experimental)


------------
Known Issues
------------

* dOpenCL does not yet support the OpenCL ICD loader mechanism.
  Hence, dOpenCL has to be preloaded using LD_PRELOAD to override the system's
  ICD loader or OpenCL implementation.

* In most cases, the dOpenCL daemon will crash if a host is disconnected (e.g.,
  due to an application failure) during a running data transfer.

* Programs currently cannot be built from binaries or built-in kernels (OpenCL
  1.2)

* Programs are always build synchronously; callbacks are supported though

* #include directives in OpenCL C programs are currently not supported

* dOpenCL does not support the following OpenCL APIs, but will support them in
  future releases:
  + all image and sampler APIs
  + all sub-buffer APIs
  + vendor-specific extensions (e.g., device fission)
  + all sub-device APIs (OpenCL 1.2)
  + clGetKernelArgInfo (OpenCL 1.2)
  + cl{Compile|Link}Program (OpenCL 1.2)
  + clEnqueue{Read|Write|Copy}BufferRect (OpenCL 1.2)
  + clEnqueueFillBuffer (OpenCL 1.2)
  + clEnqueueMigrateMemObject (OpenCL 1.2)

* dOpenCL does not and will not support the following OpenCL APIs:
  + OpenGL/CL or DirectX/CL interop
  + native kernels (clEnqueueNativeKernel is implemented but always returns
    CL_INVALID_OPERATION)
  + OpenCL 1.0 deprecated APIs

* dOpenCL collective operation APIs are not yet available

* Communication between nodes in dOpenCL is not secured in any way.

* Nodes in dOpenCL should all use the same byte order.


-------
Contact
-------

If you have any questions or suggestions regarding dOpenCL feel free to contact
me via email (philipp.kegel@uni-muenster.de).

