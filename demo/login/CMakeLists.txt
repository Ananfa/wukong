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

find_library(CORPC_MYSQL_LIB corpc_mysql)
if(NOT CORPC_MYSQL_LIB)
    message(FATAL_ERROR "corpc_mysql library not found")
endif()

find_library(MYSQLCLIENT_LIB mysqlclient /usr/local/mysql/lib /usr/lib64/mysql)
if(NOT MYSQLCLIENT_LIB)
    message(FATAL_ERROR "mysqlclient library not found")
endif()

find_library(ZOOKEEPER_LIB_ST zookeeper_st)
if(NOT ZOOKEEPER_LIB_ST)
    message(FATAL_ERROR "zookeeper_st library not found")
endif()

if (CMAKE_BUILD_TYPE)
else()
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

set(SOURCE_FILES
    ../../utils/url.cpp
    ../../utils/utility.cpp
    ../../utils/string_utils.cpp
    ../../utils/redis_utils.cpp
    ../../utils/mysql_utils.cpp
    ../../utils/proto_utils.cpp
    ../../utils/uuid_utils.cpp
    ../generate/demo_role_builder.cpp
    ../generate/demo_db_utils.cpp
    ../generate/demo_utils.cpp
    ../proto/demo.pb.cc)

# Add source files
aux_source_directory(../../proto SOURCE_FILES)
aux_source_directory(../../proto/service SOURCE_FILES)
aux_source_directory(../../share/client SOURCE_FILES)
aux_source_directory(../../share/http SOURCE_FILES)
aux_source_directory(../../login/src SOURCE_FILES)
aux_source_directory(./src SOURCE_FILES)

set(CMAKE_VERBOSE_MAKEFILE ON)

# This for mac osx only
set(CMAKE_MACOSX_RPATH 0)

# Set cflags
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=gnu++11 -fPIC -Wall -Wno-reorder -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -DDEBUG=1 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 ${CMAKE_CXX_FLAGS}")

# Add include directories
include_directories(/usr/include/mysql)
include_directories(/usr/local/include)
include_directories(/usr/local/include/co)
include_directories(/usr/local/include/corpc)
include_directories(/usr/local/include/corpc/proto)
include_directories(/usr/local/include/hiredis)
include_directories(/usr/local/include/zookeeper)
include_directories(../proto)
include_directories(../generate)
include_directories(../..)
include_directories(../../login/src)
include_directories(../../share)
include_directories(../../share/client)
include_directories(../../share/http)
include_directories(../../utils)
include_directories(../../proto)
include_directories(../../proto/service)
include_directories(../../deps/rapidjson/include)

# Add target
add_executable(logind ${SOURCE_FILES})

set(MY_LINK_LIBRARIES -L/usr/local/lib -L/usr/lib64/mysql -luuid -lhiredis -lmysqlclient -lcurl -lzookeeper_st -lprotobuf -lcorpc_pubsub -lcorpc_redis -lcorpc_mysql -lcorpc -lco -ldl)
target_link_libraries(logind ${MY_LINK_LIBRARIES})
