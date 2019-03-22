cmake_minimum_required(VERSION 3.1)
project(peoplebox)

enable_language(C)
set (CMAKE_C_STANDARD 11)

# C++ is required for CUDA activation, building and linking, if CUDA was not necessary C++ could be completely removed
enable_language(CXX)
set (CMAKE_CXX_STANDARD 14)

### Set output directories on a per-configuration base
# Single configuration
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
# Multi configuration
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

#if(EXISTS "$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
#   message(STATUS "Enabling physycom settings")
#   include("$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
#else()
#   message(STATUS "Unable to find physycom settings file")
#endif()

#darknet deps
include(darknet/CMakeLists.txt)

if(CUDA_FOUND)
  list(APPEND linked_libs ${CUDA_LIBRARIES} ${CUDA_CUBLAS_LIBRARIES} ${CUDA_curand_LIBRARY})
endif()

if(CUDNN_FOUND)
  list(APPEND linked_libs ${CUDNN_LIBRARY})
endif()

if(OPENMP_FOUND AND APPLE)
  list(APPEND linked_libs OpenMP::OpenMP_C)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
  list(APPEND linked_libs m)
endif()

if(MSVC)
  list(APPEND linked_libs ${PTHREADS_LIBRARIES} wsock32 ws2_32)
endif()

include_directories(${CMAKE_SOURCE_DIR}/jsoncons/include ${CMAKE_SOURCE_DIR}/src ${OpenCV_INCLUDE_DIRS})

# add new empty test
add_executable(test_empty ${CMAKE_SOURCE_DIR}/src/test_empty.cpp)
set_target_properties(test_empty PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_empty PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src> $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${Stb_INCLUDE_DIR}>)

if(OpenCV_FOUND)
target_link_libraries(test_empty PRIVATE ${OpenCV_LIBS})
endif()
target_link_libraries(test_empty PRIVATE darklib)

install(TARGETS test_empty DESTINATION "${CMAKE_SOURCE_DIR}")
