
// 文件资源获取及上传服务
//  与HttpServer的Get/Post与Put方法获取/上传资源文件相似，
//  HttpServer仅需将请求的资源写在url内，以/www目录的相对位置访问资源，
//  而ResourceServer的网络请求url可用于服务定位（服务名、函数名）及资源访问，
//  以/resource目录的相对位置访问资源，
//  header、content内可传递信息，content内传递的信息为json格式数据

#pragma once

#include <string>
// #include <mutex>
// #include <map>
// #include <iostream>
#include <functional>
#include <memory>
#include "TcpServer.hpp"
// #include "EventLoop.hpp"
#include "TimerManager.hpp"
#include "TcpConnection.hpp"
// #include "HttpSession.hpp"
// #include "ThreadPool.hpp"
#include "Timer.hpp"


class ResourceServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::shared_ptr<Timer> spTimer;
    ResourceServer(EventLoop *loop, const int workThreadNum = 2, ThreadPool *threadPool = NULL, const int loopThreadNum = 0, const int port = 80, TcpServer *shareTcpServer = NULL);
    ~ResourceServer();

private:
    std::string serviceName_;
    std::mutex mutex_;
    std::map<spTcpConnection, spTimer> timerList_;
    int workThreadNum_;
    int loopThreadNum_;
    int tcpServerPort_;         // tcpServer的EPOLL的服务端口
    TcpServer *tcpserver_;       // 基础网络服务TcpServer
    ThreadPool *threadpool_;    // 线程池
    void HandleMessage(spTcpConnection &sptcpconn);       // ResourceServer模式处理收到的请求
    void HandleSendComplete(spTcpConnection &sptcpconn);  // ResourceServer模式数据处理发送客户端完毕
    void HandleClose(spTcpConnection &sptcpconn);         // ResourceServer模式处理连接断开
    void HandleError(spTcpConnection &sptcpconn);         // ResourceServer模式处理连接出错

};

ResourceServer::ResourceServer(EventLoop *loop, const int workThreadNum, ThreadPool *threadPool, const int loopThreadNum, const int port, TcpServer *shareTcpServer)
    :  serviceName_("ResourceService"),
      loopThreadNum_(loopThreadNum),
      workThreadNum_(workThreadNum),
      tcpServerPort_(port),
      threadpool_(threadPool ? threadPool : (workThreadNum_ > 0 ? new ThreadPool(workThreadNum_) : NULL)),
      tcpserver_(shareTcpServer ? shareTcpServer : new TcpServer(loop, tcpServerPort_, loopThreadNum))
{
    ;
}

ResourceServer::~ResourceServer()
{
    if(workThreadNum_)
    {
        delete threadpool_;
    }
    if(tcpServerPort_)
    {
        delete tcpserver_;
    }
}
