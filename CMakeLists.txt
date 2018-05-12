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

# single crowd-counting instance
add_executable(darkcrowd ${CMAKE_SOURCE_DIR}/src/darkcrowd.c)
target_link_libraries(darkcrowd darknet_lib ${CMAKE_THREAD_LIBS_INIT} ${linked_libs})
install(TARGETS darkcrowd DESTINATION ${CMAKE_SOURCE_DIR})

# multiple people-tracking instances
file(GLOB jetson_settings ${CMAKE_SOURCE_DIR}/src/boxvars/jetson*.cmakevars)
foreach(jset IN LISTS jetson_settings)
  include(${jset})
  get_filename_component(tag ${jset} NAME_WE)
  configure_file(${CMAKE_SOURCE_DIR}/src/darktrack.c.in ${CMAKE_SOURCE_DIR}/src/darktrack_${tag}.c)
  list(APPEND physycom_${tag}_exe
    darktrack_${tag}
  )

  list(APPEND darktrack_${tag}_src ${CMAKE_SOURCE_DIR}/src/darktrack_${tag}.c)
  list(APPEND physycom_${tag}_src
    darktrack_${tag}_src
  )

  list(LENGTH physycom_${tag}_exe physycom_${tag}_len_)
  math(EXPR physycom_${tag}_len ${physycom_${tag}_len_}-1)
  foreach(i RANGE ${physycom_${tag}_len})
    list(GET physycom_${tag}_exe ${i} exe)
    list(GET physycom_${tag}_src ${i} src)
    add_executable(${exe} ${${src}})
    target_link_libraries(${exe} darknet_lib ${CMAKE_THREAD_LIBS_INIT})
  endforeach()

  foreach(${exe} IN LISTS ${physycom_${tag}_exe})
    target_link_libraries(${exe} ${linked_libs})
  endforeach()

  install(TARGETS ${physycom_${tag}_exe} DESTINATION ${CMAKE_SOURCE_DIR})
endforeach()

# tools
if(OpenCV_FOUND)
  add_executable(dump_stream ${CMAKE_SOURCE_DIR}/src/dump_stream.cpp)
  target_link_libraries(dump_stream ${OpenCV_LIBRARIES})

  file(GLOB raspi_settings ${CMAKE_SOURCE_DIR}/src/boxvars/raspi*.cmakevars)
  foreach(rset IN LISTS raspi_settings)
    include(${rset})
    get_filename_component(rtag ${rset} NAME_WE)
    configure_file(${CMAKE_SOURCE_DIR}/src/save_frame.cpp.in ${CMAKE_SOURCE_DIR}/src/save_frame_${rtag}.cpp)
    add_executable(save_frame_${rtag} ${CMAKE_SOURCE_DIR}/src/save_frame_${rtag}.cpp)
    target_link_libraries(save_frame_${rtag} ${OpenCV_LIBRARIES})
    install(TARGETS save_frame_${rtag} DESTINATION ${CMAKE_SOURCE_DIR})
  endforeach()

  install(TARGETS dump_stream DESTINATION ${CMAKE_SOURCE_DIR})
endif()
