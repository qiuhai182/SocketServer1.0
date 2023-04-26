
#include "../library/Resource.hpp"
#include "../library/LogServer.hpp"
#include "http_server.h"

/*
 * main函数，服务程序入口函数
 *
 */
int main(int argc, char *argv[])
{
    // 默认初始化参数
    int port = 80;             // 服务端口
    int iothreadnum = 100;     // EventLoop工作线程数量
    int workerthreadnum = 300; // 线程池工作线程数量
    if (argc == 4)
    {
        // 启动初始化参数
        port = atoi(argv[1]);
        iothreadnum = atoi(argv[2]);     // EventLoop工作线程数量
        workerthreadnum = atoi(argv[3]); // 线程池工作线程数量
    }

    LOG_INIT(logPath.data());   // 初始化日志类单例

    LOG(LoggerLevel::INFO, "%s\n", "即将启动HttpServer服务，开始初始化IO逻辑");
    EventLoop loop; // 该EventLoop是TcpServer的参数，其可以执行监听逻辑主函数
    LOG(LoggerLevel::INFO, "%s\n", "即将启动HttpServer服务，开始初始化业务逻辑");
    HttpServer httpServer(&loop, workerthreadnum, NULL, iothreadnum, port, nullptr);

    try
    {
        LOG(LoggerLevel::INFO, "%s\n", "启动HttpServer服务，开始监听网络请求");
        loop.loop();
    } catch (std::bad_alloc &ba) {
        LOG(LoggerLevel::ERROR, "bad_alloc caught in main： %s\n", ba.what());
        std::cerr << "bad_alloc caught in main: " << ba.what() << '\n';
    } catch (const std::exception& e) {
		std::cout << "catched exception : " << e.what() << std::endl;
	} catch (const std::string& e) {
		std::cout << "catched string : " << e.c_str() << std::endl;
	} catch (const char* e) {
		std::cout << "catched const char * :" << e << std::endl;
	} catch(...) {
		std::cout << "catched 未知异常" << std::endl;
    }

    return 0;
}
