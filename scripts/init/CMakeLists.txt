cmake_minimum_required(VERSION 3.0)

project(accountService C CXX)

#将给定的路径添加到编译器搜索包含文件（.h文件）的路径列表中，程序中可以直接include该目录下的文件
include_directories(../../library)

#搜集所有在指定路径下的源文件的文件名，输出到变量SRC_DIRS
aux_source_directory(../../library SRC_DIRS)

#指定连接器搜索库文件时的路径
link_directories(../../library/out)

#定义编译可选项
option(LINK_SO "是否属于动态链接" OFF)

#调用shell命令或脚本，设置输出管道OUT_PATH
execute_process(
    COMMAND bash -c "find ${PROJECT_SOURCE_DIR}/../../.. -type d -regex \".*output/include$\" | head -n1 | xargs dirname | tr -d '\n'"
    OUTPUT_VARIABLE OUTPUT_PATH
)

#定义要由CMake模块加载的搜索路径
set(CMAKE_PREFIX_PATH ${OUTPUT_PATH})

#定义数据库可用变量
SET(ENABLE_MYSQL ON)

if (ENABLE_MYSQL)
#为源文件的编译添加由-D引入的参数
add_definitions(-DORMPP_ENABLE_MYSQL)
endif()

#在包含上下文的范围内加载源文件
include(FindThreads)
include(FindProtobuf)

#查找指定目录的proto文件编译生成的源文件和头文件，分别输出到变量PROTO_SRC和PROTO_HEADER
# protobuf_generate_cpp(PROTO_SRC PROTO_HEADER 
#                     #   ../protobuf/account/account.proto
#                     #  ../../protobuf/bookcity/bookcity.proto
#                     #   ../protobuf/bookshelf/bookshelf.proto 
#                     #   ../protobuf/collectdata/collectdata.proto
#                     #  ../../protobuf/common/common.proto
#                     #   ../protobuf/echo/echo.proto
#                     #  ../protobuf/fileupdown/fileupdown.proto
#                     )
                    
#添加头文件包含列表，添加proto头文件
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Search for libthrift* by best effort. If it is not found and brpc is
# compiled with thrift protocol enabled, a link error would be reported.
find_library(THRIFT_LIB NAMES thrift)
if (NOT THRIFT_LIB)
    set(THRIFT_LIB "")
endif()

find_library(THRIFTNB_LIB NAMES thriftnb)
if (NOT THRIFTNB_LIB)
    set(THRIFTNB_LIB "")
endif()

find_path(RAPIDJSON_INCLUDE_PATH butil/third_party/rapidjson/rapidjson.h)
if(NOT RAPIDJSON_INCLUDE_PATH)
    message(FATAL_ERROR "Fail to find rapidjson")
endif()
include_directories(${RAPIDJSON_INCLUDE_PATH})

find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h)
if(LINK_SO)
    find_library(BRPC_LIB NAMES brpc)
else()
    find_library(BRPC_LIB NAMES libbrpc.a brpc)
endif()
if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
    message(FATAL_ERROR "Fail to find brpc")
endif()
include_directories(${BRPC_INCLUDE_PATH})

find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
    message(FATAL_ERROR "Fail to find gflags")
endif()
include_directories(${GFLAGS_INCLUDE_PATH})

execute_process(
    COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
    OUTPUT_VARIABLE GFLAGS_NS
)
if(${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
    execute_process(
        COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
        OUTPUT_VARIABLE GFLAGS_NS
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
    if(NOT HAVE_CLOCK_GETTIME)
        set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    endif()
endif()

set(CMAKE_CPP_FLAGS "${DEFINE_CLOCK_GETTIME} -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -pthread -std=c++17 -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")

if(CMAKE_VERSION VERSION_LESS "3.1.3")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif()
else()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif()
include_directories(${LEVELDB_INCLUDE_PATH})

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OPENSSL_ROOT_DIR
        "/usr/local/opt/openssl"    # Homebrew installed OpenSSL
        )
endif()
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(OpenSSL)

set(DYNAMIC_LIB
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    ${THRIFT_LIB}
    ${THRIFTNB_LIB}
    dl
    )

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(DYNAMIC_LIB ${DYNAMIC_LIB}
        pthread
	mysqlclient
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreData"
        "-framework CoreText"
        "-framework Security"
        "-framework Foundation"
        "-Wl,-U,_MallocExtension_ReleaseFreeMemory"
        "-Wl,-U,_ProfilerStart"
        "-Wl,-U,_ProfilerStop")
endif()

#使用给定的源文件及给定目录下的源文件，为工程创建一个可执行文件server
add_executable(init init.cpp ${PROTO_HEADER} ${PROTO_SRC} ${SRC_DIRS})

#添加链接库
target_link_libraries(init ${BRPC_LIB} ${DYNAMIC_LIB} lib_iReader.a mysqlclient)
#target_link_libraries(init  ${DYNAMIC_LIB} lib_iReader.a mysqlclient)
add_definitions(-w) #忽略警告


