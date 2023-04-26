cmake_minimum_required(VERSION 3.0)

project(SocketService C CXX)

# c++编译选项
set(CMAKE_CXX_FLAGS "-std=c++17")

# 添加头文件
include_directories(../../library)
include_directories(../../library/jsoncpp)

# 添加源文件
aux_source_directory(../../library SRC_DIRS_LIBRARY)
aux_source_directory(../../library/jsoncpp SRC_DIRS_LIBRARY_JSONCPP)
aux_source_directory(. SRC_DIRS_SERVICE)

# 添加cmake所在目录及其子目录的所有.h头文件
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#指定链接器搜索库文件时的路径
link_directories(../../library/out)

SET(CMAKE_BUILD_TYPE "Debug")

add_executable(server all_server.cpp ${SRC_DIRS_LIBRARY} ${SRC_DIRS_LIBRARY_JSONCPP} ${SRC_DIRS_SERVICE})

target_link_libraries(server pthread)

add_definitions(-w) # 忽略编译警告
