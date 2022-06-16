cmake_minimum_required(VERSION 2.8)
project(fix_echo)

# Check dependency libraries
find_library(LUA_LIB lua)
if(NOT LUA_LIB)
    message(FATAL_ERROR "lua library not found")
endif()

find_library(PROTOBUF_LIB protobuf /usr/local/protobuf/lib)
if(NOT PROTOBUF_LIB)
    message(FATAL_ERROR "protobuf library not found")
endif()

find_library(CO_LIB co)
if(NOT CO_LIB)
    message(FATAL_ERROR "co library not found")
endif()

find_library(CORPC_LIB corpc)
if(NOT CORPC_LIB)
    message(FATAL_ERROR "corpc library not found")
endif()

if (CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Add source files
aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCE_FILES)

set(CMAKE_VERBOSE_MAKEFILE ON)

# This for mac osx only
set(CMAKE_MACOSX_RPATH 0)

# Set cflags
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=gnu++11 -fPIC -Wall -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -DDEBUG=1 -DLOG_LEVEL=0 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DLOG_LEVEL=1 ${CMAKE_CXX_FLAGS}")

# Add include directories
include_directories(/usr/local/protobuf/include)
include_directories(/usr/local/include)
include_directories(/usr/local/include/co)
include_directories(/usr/local/include/corpc)
include_directories(/usr/local/include/corpc/proto)
include_directories(../../..)
include_directories(../../../proto)
include_directories(../../../proto/service)
include_directories(../../../utils)
include_directories(../../../share/game)
include_directories(../../../share/event)
include_directories(../../proto)
include_directories(../../generate)
include_directories(../../share)
include_directories(../../lobby/src)


# Add static and shared library target
add_library(fix_echo SHARED ${SOURCE_FILES})

# Set library output name
set_target_properties(fix_echo PROPERTIES OUTPUT_NAME fixecho)

set_target_properties(fix_echo PROPERTIES CLEAN_DIRECT_OUTPUT 1)

set_target_properties(fix_echo PROPERTIES VERSION ${LIB_VERSION} SOVERSION ${LIB_VERSION})
