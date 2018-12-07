cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(peoplebox)

enable_language(C)
enable_language(CXX)
if(ENABLE_CUDA)
  enable_language(CUDA) # do not enable after searching for OpenCV package!
endif()

if (EXISTS "$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
   message(STATUS "Enabling physycom settings")
   include("$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
else()
   message(STATUS "Unable to find physycom settings file")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules/" ${CMAKE_MODULE_PATH})

#darknet deps
include(${CMAKE_CURRENT_LIST_DIR}/darknet/CMakeLists.txt)

if(VERBOSE)
  add_definitions(-DVERBOSE)
endif()

set(JSONCONS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/jsoncons/include")
set(PEOPLEBOX_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src")
set(DARKNET_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/darknet/src")
include_directories(${JSONCONS_INCLUDE_DIR} ${PEOPLEBOX_INCLUDE_DIR} ${DARKNET_INCLUDE_DIR})

include(example.cmakevars)
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/darktrack.cpp.in ${CMAKE_CURRENT_LIST_DIR}/src/auto/darktrack.cpp)
add_executable(darktrack ${CMAKE_CURRENT_LIST_DIR}/src/auto/darktrack.cpp ${CMAKE_CURRENT_LIST_DIR}/src/darktrack_utils.cpp)

if(CMAKE_COMPILER_IS_GNUCC)
  target_link_libraries(darktrack m)
endif()
if(MSVC)
  target_link_libraries(darktrack ${PTHREADS_LIBRARY} wsock32 ws2_32)
endif()

if(OpenCV_FOUND)
  add_executable(dump_stream ${CMAKE_CURRENT_LIST_DIR}/src/dump_stream.cpp)
  target_link_libraries(dump_stream ${OpenCV_LIBS})
  install(TARGETS dump_stream DESTINATION ${CMAKE_SOURCE_DIR})

  add_executable(save_stream ${CMAKE_CURRENT_LIST_DIR}/src/save_stream.cpp)
  target_link_libraries(save_stream ${OpenCV_LIBS})
  install(TARGETS save_stream DESTINATION ${CMAKE_SOURCE_DIR})

  add_executable(dump_frame ${CMAKE_CURRENT_LIST_DIR}/src/dump_frame.cpp)
  target_link_libraries(dump_frame ${OpenCV_LIBS})
  install(TARGETS dump_frame DESTINATION ${CMAKE_SOURCE_DIR})
endif()

if(CUDNN_FOUND)
  target_link_libraries(darktrack ${CUDNN_LIBRARY})
endif()

if(OpenCV_FOUND)
  target_link_libraries(darktrack ${OpenCV_LIBS})
endif()

if(OPENMP_FOUND)
  target_link_libraries(darktrack OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
  target_link_libraries(darktrack m)
endif()

if(MSVC)
  target_link_libraries(darktrack ${PTHREADS_LIBRARY})
  target_link_libraries(darktrack wsock32 ws2_32)
endif()

target_link_libraries(darktrack Threads::Threads)
target_link_libraries(darktrack darklib)

if(OpenCV_VERSION_MAJOR GREATER_EQUAL 3)
  install(TARGETS uselib_track DESTINATION ${CMAKE_SOURCE_DIR}/)
endif()
install(TARGETS darklib uselib darktrack DESTINATION ${CMAKE_SOURCE_DIR}/)
