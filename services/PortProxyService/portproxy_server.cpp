
#include "portproxy_server.h"

/*
 * main函数，服务程序入口函数
 *
 */
int main(int argc, char *argv[])
{
    // 默认初始化参数
    int port = 80;             // 服务端口
    int iothreadnum = 100;     // EventLoop工作线程数量
    int workerthreadnum = 100; // 线程池工作线程数量
    if (argc == 4)
    {
        // 启动初始化参数
        port = atoi(argv[1]);
        iothreadnum = atoi(argv[2]);     // EventLoop工作线程数量
        workerthreadnum = atoi(argv[3]); // 线程池工作线程数量
    }

    PortProxyServer portProxyServer(workerthreadnum, iothreadnum, "0.0.0.0", port);
    portProxyServer.registerPortProxy("HttpService", "0.0.0.0", 8000);
    portProxyServer.Start();

    return 0;
}
