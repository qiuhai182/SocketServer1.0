#!/bin/bash

# 编译服务程序的可执行文件

# 编译服务 $1：服务所在目录
function compileService(){
    echo "尝试构建服务$1的可执行文件"
    if [ ! -d "../services/$1/out" ]; then
        mkdir ../services/$1/out
    else
        # 删除部分构建文件
        rm -r ../services/$1/out/*
    fi
    if [ ! -d "../services/$1/out" ]; then
        # 创建目录失败
        echo -e "\033[31m 创建目录$1/out失败 \033[0m"
        return 
    else
        cd ../services/$1/out
        cmake .. && make 
    fi
    if [ ! -x "server" ]; then
        echo -e "\033[31m 构建服务$1的可执行文件失败 \033[0m"
        return 
    else
    # 删除无用文件
        rm *pb* *ake* -r
        echo -e "\033[32m 成功构建服务$1的可执行文件 \033[0m"
    fi
    cd ../../../scripts
}

# 参数个数判断
if [ $# != 1 ] ; then
    echo -e "\033[31m 用法错误: 命令$0参数不足 \033[0m"
    echo -e "\033[31m 正确用法: $0 -parameter \033[0m"
    echo -e "\033[31m 参数范围: [-hs -as -all] \033[0m"
    exit 1;
fi
# 服务名列表，参数对应 -hs -as -all
serverArr=("HttpService" "AllService")
# 服务占用端口列表
portArr=(80 8000)
case "$1" in
		"-hs" )
            compileService  ${serverArr[0]}
			;;
        "-as" )
            compileService  ${serverArr[1]}
			;;
        "-all" )
            # 编译所有服务生成对应可执行文件
            for ((i=0;i<=6;i++))
            do
                compileService  ${serverArr[i]}
            done
			;;
		* )
			echo -e "\033[31m 参数错误: 参数$1不在适用范围 \033[0m"
			exit 1
			;;
	esac

exit 0
