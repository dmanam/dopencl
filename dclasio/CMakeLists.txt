cmake_minimum_required(VERSION 2.6)

project(dOpenCLlib)

option(BUILD_SHARED_LIBS "Build shared library" ON)

# search for Boost System Library (required by Boost.Asio)
find_package(Boost 1.35.0 COMPONENTS system REQUIRED)

# search for Doxygen
find_package(Doxygen)

# create lists of source files and headers
file(GLOB CL_WWU_HEADERS ${PROJECT_SOURCE_DIR}/include/CL *.h)
file(GLOB_RECURSE DCL_SOURCES ${PROJECT_SOURCE_DIR}/src/dcl *.cpp)
file(GLOB_RECURSE DCL_HEADERS ${PROJECT_SOURCE_DIR}/include/dcl *.h)
file(GLOB_RECURSE DCLASIO_SOURCES ${PROJECT_SOURCE_DIR}/src/dclasio *.cpp)

# include project headers
include_directories(${PROJECT_SOURCE_DIR}/include
                    ${Boost_INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIRS})


#
# dOpenCL library target
#

add_library(dcl ${DCL_SOURCES} ${DCLASIO_SOURCES})
target_link_libraries(dcl
	${Boost_LIBRARIES})

# enable OpenCL 1.1 compatibility
set_property(TARGET dcl
	PROPERTY COMPILE_DEFINITIONS
			CL_USE_DEPRECATED_OPENCL_1_1_APIS)

if(BUILD_SHARED_LIBS)
# set library version and SONAME
	set_target_properties(dcl PROPERTIES
		VERSION 0.4.0
		SOVERSION 0)
else(BUILD_SHARED_LIBS)
# compile static library using -fPIC, such that it can be linked with a shared
# library, namely the dOpenCL ICD
	set_target_properties(dcl PROPERTIES
		COMPILE_FLAGS -fPIC)
endif(BUILD_SHARED_LIBS)


#
# Documentation target
#

# TODO Warn if Doxygen has not been found
# replace CMake variable in the Doxyfile.in template thereby generating Doxyfile
configure_file(${PROJECT_SOURCE_DIR}/doc/Doxyfile.in
	${PROJECT_BINARY_DIR}/doc/Doxyfile @ONLY)
# run Doxygen in build directory using the generated Doxyfile
add_custom_target(doc
	${DOXYGEN_EXECUTABLE}
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/doc
	COMMENT "Generating dOpenCL API documentation")


#
# install configuration
#

install(TARGETS dcl
	RUNTIME DESTINATION lib
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib)
	
# TODO Install OpenCL API extension headers
#install(FILES ${CL_WWU_HEADERS} DESTINATION include/CL)
# TODO Install dOpenCL API headers
#install(FILES ${DCL_HEADERS} DESTINATION include/dcl)
# TODO Install dOpenCL API documentation
