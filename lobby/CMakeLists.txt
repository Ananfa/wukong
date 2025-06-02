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

find_library(CORPC_PUBSUB_LIB corpc_pubsub)
if(NOT CORPC_PUBSUB_LIB)
    message(FATAL_ERROR "corpc_pubsub library not found")
endif()

find_library(HIREDIS_LIB hiredis)
if(NOT HIREDIS_LIB)
    message(FATAL_ERROR "hiredis library not found")
endif()

find_library(CORPC_MYSQL_LIB corpc_mysql)
if(NOT CORPC_MYSQL_LIB)
    message(FATAL_ERROR "corpc_mysql library not found")
endif()

find_library(MYSQLCLIENT_LIB mysqlclient /usr/local/mysql/lib /usr/lib64/mysql)
if(NOT MYSQLCLIENT_LIB)
    message(FATAL_ERROR "mysqlclient library not found")
endif()

if (CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(SOURCE_FILES
    ../utils/utility.cpp
    ../utils/string_utils.cpp
    ../utils/proto_utils.cpp
    ../utils/redis_utils.cpp
    ../utils/mysql_utils.cpp
    ../demo/proto/demo.pb.cc
    ../demo/generate/demo_lobby_object_data.cpp
    ../demo/generate/demo_utils.cpp)

# Add source files
aux_source_directory(../proto SOURCE_FILES)
aux_source_directory(../proto/service SOURCE_FILES)
aux_source_directory(../proto/game SOURCE_FILES)
aux_source_directory(../share/db SOURCE_FILES)
aux_source_directory(../share/agent SOURCE_FILES)
aux_source_directory(../share/event SOURCE_FILES)
aux_source_directory(../share/log SOURCE_FILES)
aux_source_directory(../share/msghdl SOURCE_FILES)
aux_source_directory(./src SOURCE_FILES)

set(CMAKE_VERBOSE_MAKEFILE ON)

# This for mac osx only
set(CMAKE_MACOSX_RPATH 0)

# Set cflags
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-DUSE_MESSAGE_HOTFIX -std=gnu++11 -fPIC -Wall -Wno-reorder -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -DDEBUG=1 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 ${CMAKE_CXX_FLAGS}")

# Add include directories
include_directories(/usr/include/mysql)
include_directories(/usr/local/include)
include_directories(/usr/local/include/co)
include_directories(/usr/local/include/corpc)
include_directories(/usr/local/include/corpc/kcp)
include_directories(/usr/local/include/corpc/proto)
include_directories(/usr/local/include/hiredis)
include_directories(../demo/proto)
include_directories(../demo/generate)
include_directories(../demo/share)
include_directories(./src)
include_directories(..)
include_directories(../share)
include_directories(../share/agent)
include_directories(../share/db)
include_directories(../share/http)
include_directories(../share/event)
include_directories(../share/log)
include_directories(../share/msghdl)
include_directories(../utils)
include_directories(../proto)
include_directories(../proto/game)
include_directories(../proto/service)
include_directories(../deps/rapidjson/include)

# Add target
add_executable(lobbyd ${SOURCE_FILES})

set(MY_LINK_LIBRARIES -L/usr/local/lib -L/usr/lib64/mysql -llua -lhiredis -lmysqlclient -lprotobuf -lcorpc_pubsub -lcorpc_redis -lcorpc_mysql -lcorpc -lco -ldl)
target_link_libraries(lobbyd ${MY_LINK_LIBRARIES})
