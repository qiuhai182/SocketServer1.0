
#include <signal.h>
#include "../library/EventLoop.hpp"
#include "../library/EchoServer.hpp"
#include "../library/HttpServer.hpp"

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

/*
 * main函数，服务程序入口函数
 *
 */
int main(int argc, char *argv[])
{
    signal(SIGUSR1, sighandler1);
    signal(SIGUSR2, sighandler2);
    signal(SIGINT, sighandler2); // SIG_IGN = Ctrl+C
    signal(SIGPIPE, SIG_IGN);    // 忽略信号的处理程序

    // 默认初始化参数
    int port = 80;         // 服务端口
    int iothreadnum = 0;     // EventLoop工作线程数量
    int workerthreadnum = 4; // 线程池工作线程数量
    if (argc == 4)
    {
        // 启动初始化参数
        port = atoi(argv[1]);
        iothreadnum = atoi(argv[2]);        // EventLoop工作线程数量
        workerthreadnum = atoi(argv[3]);    // 线程池工作线程数量
    }

    EventLoop loop;
    HttpServer httpServer(&loop, port, iothreadnum, workerthreadnum);
    httpServer.Start();
    try
    {
        loop.loop();
    }
    catch (std::bad_alloc &ba)
    {
        std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task: " << ba.what() << '\n';
    }

    // EventLoop loop;
    // EchoServer echoServer(&loop, port, iothreadnum);
    // echoServer.Start();
    // loop.loop();

    return 0;
}
