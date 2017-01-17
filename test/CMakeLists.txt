cmake_minimum_required(VERSION 2.6)

project(dOpenCLtest)

set(DOPENCL_INCLUDE_DIR "${dOpenCLlib_SOURCE_DIR}/include" CACHE PATH "Path to dOpenCL headers")
set(DOPENCL_LIBRARY_DIR "${dOpenCLlib_BINARY_DIR}" CACHE PATH "Path to dOpenCL library")
set(DOPENCL_ICD_DIR "${dOpenCLicd_BINARY_DIR}" CACHE PATH "Path to dOpenCL ICD")

# search for Boost.Threads
find_package(Boost 1.41.0 COMPONENTS unit_test_framework REQUIRED)

include_directories(${DOPENCL_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIRS})

link_directories(${DOPENCL_ICD_DIR}
                 ${DOPENCL_LIBRARY_DIR}
                 ${Boost_LIBRARY_DIRS})

# generate dOpenCL node file
file(WRITE ${PROJECT_BINARY_DIR}/dcl.nodes
	"localhost")

# 
# dOpenCL test targets
#
# enable_testing() must be defined in the root CMakeLists.txt, otherwise 'make
# test' will not work correctly

set(UTILITY_SOURCES ${PROJECT_SOURCE_DIR}/src/utility.cpp)

add_executable(CommandQueue ${UTILITY_SOURCES} ${PROJECT_SOURCE_DIR}/src/CommandQueue.cpp)
add_executable(Context ${UTILITY_SOURCES} ${PROJECT_SOURCE_DIR}/src/Context.cpp)
add_executable(Device ${UTILITY_SOURCES} ${PROJECT_SOURCE_DIR}/src/Device.cpp)
add_executable(Event ${UTILITY_SOURCES} ${PROJECT_SOURCE_DIR}/src/Event.cpp)
add_executable(Memory
		${UTILITY_SOURCES}
		${PROJECT_SOURCE_DIR}/src/Buffer.cpp
		${PROJECT_SOURCE_DIR}/src/Mapping.cpp
		${PROJECT_SOURCE_DIR}/src/Memory.cpp
		${PROJECT_SOURCE_DIR}/src/MemoryConsistency.cpp)
add_executable(Platform ${PROJECT_SOURCE_DIR}/src/Platform.cpp)
add_executable(Program ${UTILITY_SOURCES} ${PROJECT_SOURCE_DIR}/src/Program.cpp)

foreach(test CommandQueue Context Device Event Memory Platform Program)
	add_test(${test} ${test})
	
	target_link_libraries(${test}
		dOpenCL
		dcl
		${Boost_LIBRARIES})
		
	set_property(TARGET ${test}
		APPEND PROPERTY COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK)
endforeach(test)
