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
  add_executable(darktrack_${tag} ${CMAKE_SOURCE_DIR}/src/darktrack_${tag}.c)
  list(APPEND apps darktrack_${tag})
  target_link_libraries(darktrack_${tag} darknet_lib ${linked_libs} ${CMAKE_THREAD_LIBS_INIT})
  install(TARGETS darktrack_${tag} DESTINATION ${CMAKE_SOURCE_DIR})
endforeach()

# tools
if(OpenCV_FOUND)
  add_executable(dump_stream ${CMAKE_SOURCE_DIR}/src/dump_stream.cpp)
  target_link_libraries(dump_stream ${OpenCV_LIBRARIES})
  install(TARGETS dump_stream DESTINATION ${CMAKE_SOURCE_DIR})

  add_executable(dump_frame ${CMAKE_SOURCE_DIR}/src/dump_frame.cpp)
  target_link_libraries(dump_frame ${OpenCV_LIBRARIES})
  install(TARGETS dump_frame DESTINATION ${CMAKE_SOURCE_DIR})

  file(GLOB settings ${CMAKE_SOURCE_DIR}/src/boxvars/*.cmakevars)
  foreach(set IN LISTS settings)
    include(${set})
    get_filename_component(tag ${set} NAME_WE)
    configure_file(${CMAKE_SOURCE_DIR}/src/save_frame.cpp.in ${CMAKE_SOURCE_DIR}/src/save_frame_${tag}.cpp)
    add_executable(save_frame_${tag} ${CMAKE_SOURCE_DIR}/src/save_frame_${tag}.cpp)
    list(APPEND apps save_frame_${tag})
    target_link_libraries(save_frame_${tag} ${OpenCV_LIBRARIES})
    install(TARGETS save_frame_${tag} DESTINATION ${CMAKE_SOURCE_DIR})
  endforeach()
endif()

# remote deploy (untested)
if( remote )
  message("remote          : ${remote}")
  add_custom_target(deploy_darknetlib DEPENDS darknet_lib COMMAND scp -rpd $<TARGET_LINKER_FILE:darknet_lib> ${remote})
  foreach(app IN LISTS apps)
    add_custom_target(deploy_${app} DEPENDS ${app} COMMAND scp -rpd $<TARGET_FILE:${app}> ${remote})
    list(APPEND deployable deploy_${app})
  endforeach()
  add_custom_target(deploy DEPENDS ${deployable} deploy_darknetlib)
endif()

