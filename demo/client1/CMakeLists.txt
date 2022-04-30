cmake_minimum_required(VERSION 2.8)
project(example_redis)

# Check dependency libraries
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

set(SOURCE_FILES
    ../../share/http/http_client.cpp
    ../../utils/uuid_utils.cpp
    ../../proto/common.pb.cc
    ../../proto/game/game.pb.cc
    ../proto/demo.pb.cc)

# Add source files
aux_source_directory(./src SOURCE_FILES)

set(CMAKE_VERBOSE_MAKEFILE ON)

# This for mac osx only
set(CMAKE_MACOSX_RPATH 0)

# Set cflags
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=gnu++11 -fPIC -Wall -Wno-reorder -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -DDEBUG=1 -DLOG_LEVEL=0 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DLOG_LEVEL=0 ${CMAKE_CXX_FLAGS}")

# Add include directories
include_directories(/usr/local/protobuf/include)
include_directories(/usr/local/include)
include_directories(/usr/local/include/co)
include_directories(/usr/local/include/corpc)
include_directories(/usr/local/include/corpc/proto)
include_directories(../proto)
include_directories(../share)
include_directories(../..)
include_directories(../../share)
include_directories(../../share/http)
include_directories(../../utils)
include_directories(../../proto)
include_directories(../../proto/game)
include_directories(../../deps/rapidjson/include)

# Add target
add_executable(demo_cli ${SOURCE_FILES})

set(MY_LINK_LIBRARIES -L/usr/local/lib -luuid -lcurl -lprotobuf -lcorpc -lco -ldl)
target_link_libraries(demo_cli ${MY_LINK_LIBRARIES})
