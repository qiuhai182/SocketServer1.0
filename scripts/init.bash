#!/bin/bash

# 执行初始化程序的编译并运行

# 创建lib_iReader.a
function link_lib(){
    if [ ! -d "../library/out" ]; then
        mkdir ../library/out
    fi
    if [ ! -f "../library/out/lib_iReader.a" ]; then
        cd ../library/out && cmake .. && make
    fi
    if [ -f "../library/out/lib_iReader.a" ]; then
        echo -e "\033[32m 已成功构建静态库lib_iReader.a \033[0m"
    else
        echo -e "\033[31m 未成功构建静态库lib_iReader.a \033[0m"
        exit 1
    fi
}

# 编译初始化程序
function compile_init(){
    if [ ! -d "init/out" ]; then
        mkdir init/out/
    else
        # 删除部分构建文件
        rm -r init/out/*
    fi
    if [ ! -d "init/out/" ]; then
        # 创建失败
        echo -e "\033[47;31m 未成功创建目录qReader/scripts/init/out \033[0m"
        exit 1 
    else
        # 构建文件
        cd init/out/
        cmake .. && make 
    fi
    if [ ! -x "init" ]; then
        echo -e "\033[47;31m 构建初始化程序qReader/scripts/init/out/init失败 \033[0m"
        exit 1
    else
    # 删除多余文件
        rm *ake* -r
        echo -e "\033[32m 成功构建初始化程序qReader/scripts/init/out/init \033[0m"
    fi
    
}

# link_lib

echo "尝试构建初始化程序"
compile_init
echo -e "\033[32m 开始执行初始化程序 \033[0m"
./init
echo -e "\033[32m 初始化完毕 \033[0m"



