#!/bin/bash

# 寻找并关闭服务进程

# 杀死进程 $1：service-服务名  $2：port-服务端口
function killProcess(){
    res="$(lsof -i:$2)"
    list=($res)
    pid=${list[10]}
    if [ ! "$pid" = "" ]; then 
        kill -9 $pid
        echo -e "\033[32m 已成功关闭服务$1 \033[0m"
    else 
        echo -e "\033[31m 未检索到服务$1，某些特殊设备无法正常检测端口 \033[0m"
    fi 
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
            killProcess  ${serverArr[0]}  ${portArr[0]}
			;;
        "-as" )
            killProcess  ${serverArr[1]}  ${portArr[1]}
			;;
        "-all" )
            # 检索并关闭所有服务
            for ((i=0;i<=6;i++))
            do
                killProcess  ${serverArr[i]}  ${portArr[i]}
            done
			;;
		* )
			echo -e "\033[31m 参数错误: 参数$1不在适用范围 \033[0m"
			exit 1
			;;
	esac

exit 0
