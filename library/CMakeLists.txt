cmake_minimum_required(VERSION 3.0)

# 编译生成的静态库文件名为：lib_SocketServerLibrary.a
project(_SocketServerLibrary)

#搜索当前目录下所有源文件，保存到变量DIR_LIB_SRC_FILES
aux_source_directory(. DIR_LIB_SRC_FILES)

# #设置c++编译选项（c++17、线程库）
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")

# #启动对C++17标准的支持
# set(CMAKE_CXX_STANDARD 17)

# #显式要求指明支持C++标准
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# #定义数据库可用变量
# set(ENABLE_MYSQL ON)

# if(ENABLE_MYSQL)
# #为源文件的编译添加由-D引入的参数
# add_definitions(-DORMPP_ENABLE_MYSQL)
# endif()

# if(ENABLE_MYSQL)
# #添加链接库，不可以在_iReader库被添加之前调用
# target_link_libraries(_SocketServerLibrary mysqlclient)
# endif()

#使用DIR_LIB_SRC_FILES里的源文件为项目添加_iReader库
add_library(_SocketServerLibrary ${DIR_LIB_SRC_FILES})

