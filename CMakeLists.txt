cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(peoplebox)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)

if (EXISTS "$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
   message(STATUS "Enabling physycom settings")
   include("$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
else()
   message(STATUS "Unable to find physycom settings file")
endif()

find_package(Darknet REQUIRED)
#find_package(jsoncons REQUIRED)
find_package(OpenCV REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads REQUIRED)
if(MSVC)
  find_package(pthreads REQUIRED)
endif()

if(VERBOSE)
  add_definitions(-DVERBOSE)
endif()

set(PEOPLEBOX_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src")
include_directories(${PEOPLEBOX_INCLUDE_DIR})

add_executable(dump_stream ${CMAKE_CURRENT_LIST_DIR}/src/dump_stream.cpp)
target_link_libraries(dump_stream PRIVATE ${OpenCV_LIBS})
install(TARGETS dump_stream DESTINATION ${CMAKE_SOURCE_DIR})

add_executable(save_stream ${CMAKE_CURRENT_LIST_DIR}/src/save_stream.cpp)
target_link_libraries(save_stream PRIVATE ${OpenCV_LIBS})
install(TARGETS save_stream DESTINATION ${CMAKE_SOURCE_DIR})

add_executable(dump_frame ${CMAKE_CURRENT_LIST_DIR}/src/dump_frame.cpp)
target_link_libraries(dump_frame PRIVATE ${OpenCV_LIBS})
install(TARGETS dump_frame DESTINATION ${CMAKE_SOURCE_DIR})

include(example.cmakevars)
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/darktrack.cpp.in ${CMAKE_CURRENT_LIST_DIR}/src/auto/darktrack.cpp)
add_executable(darktrack ${CMAKE_CURRENT_LIST_DIR}/src/auto/darktrack.cpp ${CMAKE_CURRENT_LIST_DIR}/src/darktrack_utils.cpp)
target_link_libraries(darktrack PRIVATE Darknet::dark)
if(MSVC)
  target_link_libraries(darktrack PRIVATE PThreads4W::PThreads4W)
endif()
target_link_libraries(darktrack PRIVATE Threads::Threads)
#target_link_libraries(darktrack jsoncons)
install(TARGETS darktrack DESTINATION ${CMAKE_SOURCE_DIR})
