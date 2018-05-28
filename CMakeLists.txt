cmake_minimum_required(VERSION 3.1)
project(peoplebox C CXX)

if(EXISTS "$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
   message(STATUS "Enabling physycom settings")
   include("$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
else()
   message(STATUS "Unable to find physycom settings file")
endif()

#darknet deps
include(darknet/CMakeLists.txt)

if(VERBOSE)
  add_definitions(-DVERBOSE)
endif()

if(CUDA_FOUND)
  list(APPEND linked_libs darknet_cuda ${CUDA_LIBRARIES} ${CUDA_CUBLAS_LIBRARIES} ${CUDA_curand_LIBRARY})
endif()

if(CUDNN_FOUND)
  list(APPEND linked_libs ${CUDNN_LIBRARY})
endif()

if(OpenCV_FOUND)
  list(APPEND linked_libs ${OpenCV_LIBRARIES})
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

include_directories(${CMAKE_SOURCE_DIR}/src/)

# crowd-counting
add_executable(darkcrowd ${CMAKE_SOURCE_DIR}/src/darkcrowd.c)
target_link_libraries(darkcrowd darknet_lib ${CMAKE_THREAD_LIBS_INIT} ${linked_libs})
install(TARGETS darkcrowd DESTINATION ${CMAKE_SOURCE_DIR})

# tracking
add_library(config ${CMAKE_SOURCE_DIR}/src/config.cpp)
add_executable(darktrack ${CMAKE_SOURCE_DIR}/src/darktrack.c ${CMAKE_SOURCE_DIR}/src/tracking.c)
target_link_libraries(darktrack config darknet_lib ${linked_libs})
install(TARGETS darktrack DESTINATION ${CMAKE_SOURCE_DIR})

# tools
add_custom_target(tools)
if(OpenCV_FOUND)
  add_executable(save_frame ${CMAKE_SOURCE_DIR}/src/save_frame.cpp)
  target_link_libraries(save_frame config ${linked_libs})
  install(TARGETS save_frame DESTINATION ${CMAKE_SOURCE_DIR})

  add_executable(dump_stream ${CMAKE_SOURCE_DIR}/src/dump_stream.cpp)
  target_link_libraries(dump_stream ${OpenCV_LIBRARIES})
  install(TARGETS dump_stream DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tools dump_stream)

  add_executable(dump_frame ${CMAKE_SOURCE_DIR}/src/dump_frame.cpp)
  target_link_libraries(dump_frame ${OpenCV_LIBRARIES})
  install(TARGETS dump_frame DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tools dump_frame)
endif()
