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

#darknet deps
include(darknet/CMakeLists.txt)

#if(CUDA_FOUND)
#  list(APPEND linked_libs ${CUDA_LIBRARIES} ${CUDA_CUBLAS_LIBRARIES} ${CUDA_curand_LIBRARY})
#endif()
#
#if(CUDNN_FOUND)
#  list(APPEND linked_libs ${CUDNN_LIBRARY})
#endif()

#if(OPENMP_FOUND AND APPLE)
#  list(APPEND linked_libs OpenMP::OpenMP_C)
#endif()

if(CMAKE_COMPILER_IS_GNUCC)
  list(APPEND linked_libs m)
endif()

if(MSVC)
  list(APPEND linked_libs ${PTHREADS_LIBRARIES})
endif()

include_directories(${CMAKE_SOURCE_DIR}/jsoncons/include ${CMAKE_SOURCE_DIR}/src ${OpenCV_INCLUDE_DIRS})

# apps
add_executable(demo ${CMAKE_SOURCE_DIR}/src/demo.cpp)
set_target_properties(demo PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(demo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src> $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${Stb_INCLUDE_DIR}>)

add_executable(doublecam ${CMAKE_SOURCE_DIR}/src/doublecam.cpp)
set_target_properties(doublecam PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(doublecam PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src> $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${Stb_INCLUDE_DIR}>)

add_executable(scan ${CMAKE_SOURCE_DIR}/src/scan.cpp)
set_target_properties(scan PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(scan PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src> $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${Stb_INCLUDE_DIR}>)


if(OpenCV_FOUND)
  target_link_libraries(demo PRIVATE ${OpenCV_LIBS})
  target_link_libraries(doublecam PRIVATE ${OpenCV_LIBS})
  target_link_libraries(scan PRIVATE ${OpenCV_LIBS})
endif()
target_link_libraries(demo PRIVATE darklib)
target_link_libraries(doublecam PRIVATE darklib)
target_link_libraries(scan PRIVATE darklib)

install(TARGETS demo DESTINATION "${CMAKE_SOURCE_DIR}")
install(TARGETS doublecam DESTINATION "${CMAKE_SOURCE_DIR}")
install(TARGETS scan DESTINATION "${CMAKE_SOURCE_DIR}")
