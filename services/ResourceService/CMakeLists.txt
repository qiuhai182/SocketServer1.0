cmake_minimum_required(VERSION 3.0)


project(SocketService C CXX)


# c++编译选项
set(CMAKE_CXX_FLAGS "-std=c++17")


# 添加头文件
include_directories(../../library)
# 添加源文件
aux_source_directory(../../library SRC_DIRS_LIBRARY)
aux_source_directory(. SRC_DIRS_SERVICE)
# 添加cmake所在目录及其子目录的所有.h头文件
include_directories(${CMAKE_CURRENT_BINARY_DIR})


# # 调用shell命令或脚本，指定系统include文件目录
# execute_process(
#     COMMAND bash -c "find ${PROJECT_SOURCE_DIR}/../../../../ -type d -regex \".*output/include$\" | head -n1 | xargs dirname | tr -d '\n'"
#     OUTPUT_VARIABLE OUTPUT_PATH)
# # 定义要由CMake模块加载的搜索路径
# set(CMAKE_PREFIX_PATH ${OUTPUT_PATH})


# # 定义数据库可用变量
# SET(ENABLE_MYSQL ON)
# if (ENABLE_MYSQL)
# # 为源文件的编译添加由-D引入的参数
# add_definitions(-D ORMPP_ENABLE_MYSQL)
# endif()


# 在包含上下文的范围内加载源文件
include(FindProtobuf)
# 查找指定目录的proto文件编译生成的源文件和头文件，分别输出到变量PROTO_SRC和PROTO_HEADER
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER 
                      ../../protobuf/account/account.proto
                      ../../protobuf/common/common.proto
                      ../../protobuf/echo/echo.proto
                      ../../protobuf/fileupdown/fileupdown.proto
                      ../../protobuf/service/service.proto
                      ../../protobuf/character/character.proto
                      ../../protobuf/relationship/relationship.proto
                    )

SET(CMAKE_BUILD_TYPE "Debug")

# add_executable(server resource_server.cpp ${PROTO_HEADER} ${PROTO_SRC} ${SRC_DIRS_LIBRARY} ${SRC_DIRS_SERVICE})
add_executable(server resource_server.cpp ${SRC_DIRS_LIBRARY} ${SRC_DIRS_SERVICE})

# target_link_libraries(server ${BRPC_LIB} ${PROTOBUF_LIBRARIES} mysqlclient hiredis pthread)
target_link_libraries(server ${BRPC_LIB} ${PROTOBUF_LIBRARIES} mysqlclient pthread)

add_definitions(-w) # 忽略编译警告



