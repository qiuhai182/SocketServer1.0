#pragma once

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <csignal>
#include "../../library/HttpServer.hpp"

EventLoop *lp; // 事件池指针

/*
 * 信号处理函数，SIGUSR1->退出服务器程序
 *
 */
static void sighandler1(int sig_no)
{
    exit(0);
}

/*
 * 信号处理函数，SIGUSR2->关闭服务
 *
 */
static void sighandler2(int sig_no)
{
    lp->Quit();
}


#endif