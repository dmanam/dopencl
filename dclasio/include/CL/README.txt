*** WARNING ***
This folder must not contain the standard OpenCL headers, but only the headers of the dOpenCL API extension.
*** WARNING ***

This folder is specified as an additional include directory for all dOpenCL
projects, as these require the definition of the dOpenCL API extension. If any
standard OpenCL header is also added to this folder it overrides the system's
OpenCL headers. This causes serious linker issues in the daemon, when the OpenCL
version of the headers (in this folder) and the system's installed OpenCL
implementation differ.
The daemon project is implemented in such a way that it implements all OpenCL
API functions that are provided on the local system (detected using the
CL_VERSION_... macros). If the OpenCL header version is greater than the version
of the system's OpenCL implementation, the linker will not find all required
symbols for linking. Some symbol are implicitly required by the OpenCL C++
binding, even if the daemon is not using these symbols directly.

Make sure that the OpenCL headers and OpenCL C++ binding are installed that
correspond to the target system's OpenCL implementation.