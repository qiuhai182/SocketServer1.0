
#include "all_server.h"

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
    int port = 8000;          // 服务端口
    int iothreadnum = 5;     // EventLoop工作线程数量
    int workerthreadnum = 5; // 线程池工作线程数量
    if (argc == 4)
    {
        // 启动初始化参数
        port = atoi(argv[1]);
        iothreadnum = atoi(argv[2]);     // EventLoop工作线程数量，负责连接监听与数据收发
        workerthreadnum = atoi(argv[3]); // 线程池工作线程数量，负责收到的请求数据进行逻辑处理
    }

    LOG_INIT(logPath.data());   // 初始化日志类单例

    EventLoop loop; // 多服务共享EventLoop，该EventLoop是TcpServer的参数，其可以执行监听逻辑主函数
    TcpServer tcpServer(&loop, port, iothreadnum); // 多服务共享基于EPOLL的TcpServer
    ThreadPool threadPool(workerthreadnum); // 多服务共享线程池
    threadPool.Start();

    // 网站式HttpServer服务一般部署于80端口，此处HttpServer共享线程池threadPool，但独立创建一个监听80端口的TcpServer
    HttpServer httpServer(&loop, 0, &threadPool, iothreadnum, 80, nullptr); // NULL与nullptr相等
    // 以下多个服务构造时使用共享的tcpServer和threadPool，则其相应构造参数可以置为0
    // 这样做的好处是析构时每个服务内部可根据构造参数判断是否需要delete对象指针：tcpServer和threadPool
    ResourceServer resourceServer(&loop, 0, &threadPool, 0, 0, &tcpServer);

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
