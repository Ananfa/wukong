cmake_minimum_required(VERSION 2.8)
project(example_redis)

# Check dependency libraries
find_library(PROTOBUF_LIB protobuf)
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

find_library(CORPC_REDIS_LIB corpc_redis)
if(NOT CORPC_REDIS_LIB)
    message(FATAL_ERROR "corpc_redis library not found")
endif()

find_library(HIREDIS_LIB hiredis)
if(NOT HIREDIS_LIB)
    message(FATAL_ERROR "hiredis library not found")
endif()

if (CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(SOURCE_FILES
    ../../utils/url_utils.cpp
    ../../utils/utility.cpp
    ../../utils/string_utils.cpp
    ../../utils/redis_utils.cpp
    ../../utils/proto_utils.cpp
    ../../share/db/redis_pool.cpp)

# Add source files
aux_source_directory(../../proto SOURCE_FILES)
aux_source_directory(../../proto/service SOURCE_FILES)
aux_source_directory(../../share/agent SOURCE_FILES)
aux_source_directory(../../share/http SOURCE_FILES)
aux_source_directory(../../opctrl/src SOURCE_FILES)
aux_source_directory(./src SOURCE_FILES)

set(CMAKE_VERBOSE_MAKEFILE ON)

# This for mac osx only
set(CMAKE_MACOSX_RPATH 0)

# Set cflags
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=gnu++11 -fPIC -Wall -Wno-reorder -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -DDEBUG=1 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 ${CMAKE_CXX_FLAGS}")

# Add include directories
include_directories(/usr/local/include)
include_directories(/usr/local/include/co)
include_directories(/usr/local/include/corpc)
include_directories(/usr/local/include/corpc/kcp)
include_directories(/usr/local/include/corpc/proto)
include_directories(/usr/local/include/hiredis)
include_directories(../proto)
include_directories(../..)
include_directories(../../opctrl/src)
include_directories(../../share)
include_directories(../../share/agent)
include_directories(../../share/db)
include_directories(../../share/http)
include_directories(../../utils)
include_directories(../../proto)
include_directories(../../proto/service)
include_directories(../../deps/rapidjson/include)

# Add target
add_executable(opctrld ${SOURCE_FILES})

set(MY_LINK_LIBRARIES -L/usr/local/lib -lhiredis -lcurl -lprotobuf -lcorpc_pubsub -lcorpc_redis -lcorpc -lco -ldl)
target_link_libraries(opctrld ${MY_LINK_LIBRARIES})
