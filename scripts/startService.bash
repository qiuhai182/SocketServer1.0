#!/bin/bash

# 启动服务并挂起在后台

function startAndHangUp(){
    echo "尝试启动服务$1并挂起"
    res="$(lsof -i:$2)"
    list=($res)
    pid=${list[10]}
    if [ ! "$pid" = "" ];then
        echo -e "\033[31m 端口$2已被占用，启动服务$1失败 \033[0m"
        return 
    else
        # 启动服务，挂起到后台，标准输出、标准错误输出重定向到log文件
        cd ../services/$1/out
        nohup ./server > $1.log 2>&1 &
    fi
    # sleep 0.1
    # 检测运行
    res="$(lsof -i:$2)"
    list=($res)
    pid=${list[10]}
    if [  "$pid" = "" ];then
        echo -e "\033[31m 启动服务$1可能不成功，某些特殊设备无法正常检测端口 \033[0m"
        return 
    else
        echo -e "\033[32m 成功启动服务$1 \033[0m"
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
            startAndHangUp  ${serverArr[0]}  ${portArr[0]}
			;;
        "-as" )
            startAndHangUp  ${serverArr[1]}  ${portArr[1]}
			;;
        "-all" )
            #全部终止
            for ((i=0;i<=6;i++))
            do
                startAndHangUp  ${serverArr[i]}  ${portArr[i]}
            done
			;;
		* )
			echo -e "\033[31m 参数错误: 参数$1不在适用范围 \033[0m"
			exit 1
			;;
	esac

exit 0
