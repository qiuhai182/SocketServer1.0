
#include "resource_server.h"

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
    int port = 8005;          // 服务端口
    int iothreadnum = 20;     // EventLoop工作线程数量
    int workerthreadnum = 20; // 线程池工作线程数量
    if (argc == 4)
    {
        // 启动初始化参数
        port = atoi(argv[1]);
        iothreadnum = atoi(argv[2]);     // EventLoop工作线程数量
        workerthreadnum = atoi(argv[3]); // 线程池工作线程数量
    }

    EventLoop loop; // 该EventLoop是TcpServer的参数，其可以执行监听逻辑主函数
    HttpServer httpServer(&loop, workerthreadnum, nullptr, iothreadnum, port, NULL);

    try
    {
        loop.loop();
    }
    catch (std::bad_alloc &ba)
    {
        std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task: " << ba.what() << '\n';
    }

    return 0;
}
