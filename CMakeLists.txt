cmake_minimum_required(VERSION 3.0)
project(mds_proxy)

if (NOT WIN32)
    message(FATAL_ERROR "not support this platform!!!")
endif()

set(BUILD_TESTS ON CACHE BOOL "Build tests program.")
#尽量编译成动态库
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries")
set(INSTALL_TEMP ON CACHE BOOL "Install project to /tmp/install")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

if ("${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "debug")
    add_definitions(-DDEBUG)
endif()

include(./cmake/Utils.cmake)
include(./cmake/FindWs32.cmake)
include(./cmake/FindExtLib.cmake)

set_win32_options()

# 设置库和可执行文件输出路径
set(OBJ_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/output)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OBJ_OUTPUT_DIR}) 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OBJ_OUTPUT_DIR}) 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OBJ_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OBJ_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OBJ_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OBJ_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OBJ_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OBJ_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OBJ_OUTPUT_DIR})

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTS)
   add_subdirectory(tests)
endif()

message(STATUS "CMAKE_SYSTEM:             " ${CMAKE_SYSTEM} )
message(STATUS "CMAKE_SYSTEM_NAME:        " ${CMAKE_SYSTEM_NAME} )
message(STATUS "CMAKE_SYSTEM_VERSION:     " ${CMAKE_SYSTEM_VERSION} )
message(STATUS "CMAKE_SYSTEM_PROCESSOR:   " ${CMAKE_SYSTEM_PROCESSOR} )
message(STATUS "CMAKE_C_FLAGS:            " ${CMAKE_C_FLAGS} )
message(STATUS "CMAKE_CXX_FLAGS:          " ${CMAKE_CXX_FLAGS} )
message(STATUS "CMAKE_BUILD_TYPE:         " ${CMAKE_BUILD_TYPE} )
message(STATUS "CMAKE_C_COMPILER:         " ${CMAKE_C_COMPILER} )
message(STATUS "CMAKE_C_COMPILER_ID:      " ${CMAKE_C_COMPILER_ID})
message(STATUS "CMAKE_CXX_COMPILER:       " ${CMAKE_CXX_COMPILER} )
message(STATUS "CMAKE_CXX_COMPILER_ID:    " ${CMAKE_CXX_COMPILER_ID})
