cmake_minimum_required(VERSION 3.1)
project(peoplebox)

enable_language(C)
set (CMAKE_C_STANDARD 11)

# C++ is required for CUDA activation, building and linking, if CUDA was not necessary C++ could be completely removed
enable_language(CXX)
set (CMAKE_CXX_STANDARD 14)


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

set(JSONCONS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/jsoncons/include)
include_directories(${JSONCONS_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

file(GLOB jetson_settings ${CMAKE_SOURCE_DIR}/src/boxvars/jetson*.cmakevars)
file(GLOB raspi_settings ${CMAKE_SOURCE_DIR}/src/boxvars/raspi*.cmakevars)
file(GLOB boat_settings ${CMAKE_SOURCE_DIR}/src/boxvars/boat*.cmakevars)
file(GLOB car_settings ${CMAKE_SOURCE_DIR}/src/boxvars/car*.cmakevars)

# crowdcounting
add_executable(json_triplet_packer ${CMAKE_SOURCE_DIR}/src/json_triplet_packer.cpp)
install(TARGETS json_triplet_packer DESTINATION ${CMAKE_SOURCE_DIR})
add_custom_target(crowdcounting)
foreach(jset IN LISTS raspi_settings)
  include(${jset})
  get_filename_component(tag ${jset} NAME_WE)
  configure_file(${CMAKE_SOURCE_DIR}/src/darkcrowd.c.in ${CMAKE_SOURCE_DIR}/src/auto/darkcrowd_${tag}.c)
  add_executable(darkcrowd_${tag} ${CMAKE_SOURCE_DIR}/src/auto/darkcrowd_${tag}.c ${CMAKE_SOURCE_DIR}/src/darktrack_utils.c)
  target_link_libraries(darkcrowd_${tag} darknet_lib ${linked_libs} ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(darkcrowd_${tag} PROPERTIES LINKER_LANGUAGE C)
  install(TARGETS darkcrowd_${tag} DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(crowdcounting darkcrowd_${tag})
endforeach()

# tracking humans
add_custom_target(tracking_humans)
foreach(jset IN LISTS jetson_settings)
  include(${jset})
  get_filename_component(tag ${jset} NAME_WE)
  configure_file(${CMAKE_SOURCE_DIR}/src/darktrack.c.in ${CMAKE_SOURCE_DIR}/src/auto/darktrack_${tag}.c)
  add_executable(darktrack_${tag} ${CMAKE_SOURCE_DIR}/src/auto/darktrack_${tag}.c ${CMAKE_SOURCE_DIR}/src/darktrack_utils.c)
  target_link_libraries(darktrack_${tag} darknet_lib ${linked_libs} ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(darktrack_${tag} PROPERTIES LINKER_LANGUAGE C)
  install(TARGETS darktrack_${tag} DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tracking_humans darktrack_${tag})
endforeach()

# tracking boats
add_custom_target(tracking_boats)
foreach(jset IN LISTS boat_settings)
  include(${jset})
  get_filename_component(tag ${jset} NAME_WE)
  configure_file(${CMAKE_SOURCE_DIR}/src/darkboat.c.in ${CMAKE_SOURCE_DIR}/src/auto/darkboat_${tag}.c)
  add_executable(darkboat_${tag} ${CMAKE_SOURCE_DIR}/src/auto/darkboat_${tag}.c ${CMAKE_SOURCE_DIR}/src/darktrack_utils.c)
  target_link_libraries(darkboat_${tag} darknet_lib ${linked_libs} ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(darkboat_${tag} PROPERTIES LINKER_LANGUAGE C)
  install(TARGETS darkboat_${tag} DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tracking_boats darkboat_${tag})
endforeach()

# tracking cars
add_custom_target(tracking_cars)
foreach(jset IN LISTS boat_settings)
  include(${jset})
  get_filename_component(tag ${jset} NAME_WE)
  configure_file(${CMAKE_SOURCE_DIR}/src/darkcar.c.in ${CMAKE_SOURCE_DIR}/src/auto/darkcar_${tag}.c)
  add_executable(darkcar_${tag} ${CMAKE_SOURCE_DIR}/src/auto/darkcar_${tag}.c ${CMAKE_SOURCE_DIR}/src/darktrack_utils.c)
  target_link_libraries(darkcar_${tag} darknet_lib ${linked_libs} ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(darkcar_${tag} PROPERTIES LINKER_LANGUAGE C)
  install(TARGETS darkcar_${tag} DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tracking_boats darkcar_${tag})
endforeach()

add_custom_target(tools)
if(OpenCV_FOUND)
  # tools
  foreach(set IN LISTS jetson_settings raspi_settings)
    include(${set})
    get_filename_component(tag ${set} NAME_WE)
    configure_file(${CMAKE_SOURCE_DIR}/src/save_frame.cpp.in ${CMAKE_SOURCE_DIR}/src/auto/save_frame_${tag}.cpp)
    add_executable(save_frame_${tag} ${CMAKE_SOURCE_DIR}/src/auto/save_frame_${tag}.cpp)
    target_link_libraries(save_frame_${tag} ${OpenCV_LIBRARIES})
    install(TARGETS save_frame_${tag} DESTINATION ${CMAKE_SOURCE_DIR})
    add_dependencies(tools save_frame_${tag})
  endforeach()

  add_executable(dump_stream ${CMAKE_SOURCE_DIR}/src/dump_stream.cpp)
  target_link_libraries(dump_stream ${OpenCV_LIBRARIES})
  install(TARGETS dump_stream DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tools dump_stream)

  add_executable(save_stream ${CMAKE_SOURCE_DIR}/src/save_stream.cpp)
  target_link_libraries(save_stream ${OpenCV_LIBRARIES})
  install(TARGETS save_stream DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tools save_stream)

  add_executable(dump_frame ${CMAKE_SOURCE_DIR}/src/dump_frame.cpp)
  target_link_libraries(dump_frame ${OpenCV_LIBRARIES})
  install(TARGETS dump_frame DESTINATION ${CMAKE_SOURCE_DIR})
  add_dependencies(tools dump_frame)
endif()
