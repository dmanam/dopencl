cmake_minimum_required(VERSION 2.6)

project(dOpenCL)

# The following code snippet for enabling C++11 support for GCC has been taken from:
# http://stackoverflow.com/questions/12715005/add-c0x-support-in-cmake
# For other compilers, C++11 support has to enabled manually
# NOTE: Modification of CMAKE_CXX_FLAGS is not shown in ncurses UI
if(CMAKE_COMPILER_IS_GNUCXX)
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GNUCXX_VERSION)
	if(GNUCXX_VERSION VERSION_EQUAL 4.6)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	elseif(GNUCXX_VERSION VERSION_GREATER 4.6)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	else()
		message(SEND_ERROR "Compiler does not support C++11")
	endif()
endif(CMAKE_COMPILER_IS_GNUCXX)

option(BUILD_UNIT_TESTS "Build dOpenCL unit tests (experimental)" OFF)

#
# dOpenCL version information
#
# Note that the version number refers to the dOpenCL ICD, daemon, and the
# dOpenCL API. The dOpenCL library may have a different version number.
#
set(DCL_VERSION_MAJOR 0)
set(DCL_VERSION_MINOR 4)
set(DCL_VERSION_PATCH 0)
set(DCL_VERSION ${DCL_VERSION_MAJOR}.${DCL_VERSION_MINOR}.${DCL_VERSION_PATCH})

add_subdirectory (dclasio)
add_subdirectory (daemon)
add_subdirectory (icdpp)

if(BUILD_UNIT_TESTS)
	# enable_testing must be defined in the root CMakeLists.txt, otherwise
	# 'make test' will not work correctly
	enable_testing ()
	add_subdirectory (test)
endif(BUILD_UNIT_TESTS)

# TODO Export targets from installation rather than build tree
#export(TARGETS dcl dOpenCL dcld FILE dOpenCL.cmake)


#
# package configuration
#
# TODO Add 'README_...txt' and 'INSTALL_...txt' to package
# TODO Add copyright and license header to all source files
# TODO Add dOpenCL API headers to package
# TODO Add dOpenCL API documentation to package

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "dOpenCL is a distributed implementation of the OpenCL API.")
set(CPACK_PACKAGE_VENDOR "Group Parallel and Distributed Systems, Department of Computer Science, University of Muenster, Germany")
set(CPACK_PACKAGE_COPYRIGHT_FILE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${DCL_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DCL_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DCL_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "dOpenCL-${DCL_VERSION_MAJOR}.${DCL_VERSION_MINOR}")
set(CPACK_PACKAGE_FILE_NAME "dOpenCL-${DCL_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}")

# WARNING:
# CPack TGZ generator will include *all* files from the source folder by default
# Checkout a clean tree from the repository to create a source package.
set(CPACK_SOURCE_IGNORE_FILES
	"/build;/.svn/;.*~$;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "dOpenCL-${DCL_VERSION}-src")
set(CPACK_SOURCE_STRIP_FILES TRUE)

if(UNIX)
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README_Linux.txt")
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
endif(UNIX)
if(WIN32)
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README_Win.txt")
set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR "ZIP")
endif(WIN32)

# CPack must be included *after* the CPack configuration
include(CPack)

