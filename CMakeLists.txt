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

# physycom
include(${CMAKE_SOURCE_DIR}/src/peoplebox.cmakevars)
configure_file(${CMAKE_SOURCE_DIR}/src/peoplebox_params.h.in ${CMAKE_SOURCE_DIR}/src/peoplebox_params.h)

list(APPEND physycom_exe
  darkcrowd
  darktrack
)

list(APPEND darkcrowd_src   ${CMAKE_SOURCE_DIR}/src/darkcrowd.c)
list(APPEND darktrack_src   ${CMAKE_SOURCE_DIR}/src/darktrack.c)
list(APPEND physycom_src
  darkcrowd_src
  darktrack_src
)

list(LENGTH physycom_exe physycom_len1)
math(EXPR physycom_len physycom_len1)
foreach(i RANGE ${physycom_len})
  list(GET physycom_exe ${i} exe)
  list(GET physycom_src ${i} src)
  add_executable(${exe} ${${src}})
  target_link_libraries(${exe} darknet_lib ${CMAKE_THREAD_LIBS_INIT})
endforeach()

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

foreach(${exe} IN LISTS ${physycom_exe})
  target_link_libraries(${exe} ${linked_libs})
endforeach()

install(TARGETS ${physycom_exe} DESTINATION ${CMAKE_SOURCE_DIR})

# tools
if(OpenCV_FOUND)
  add_executable(dump_stream ${CMAKE_SOURCE_DIR}/src/dump_stream.cpp)
  target_link_libraries(dump_stream ${OpenCV_LIBRARIES})
  install(TARGETS dump_stream DESTINATION ${CMAKE_SOURCE_DIR})
endif()
