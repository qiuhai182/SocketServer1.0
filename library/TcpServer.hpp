
// tcpServer类：
//  实现基于socket的网络服务，管理所有的tcp连接实例TcpConnection
//  是其他一切网络服务的基础服务提供类
//  tcpServer内部生成一个Channel实例用于监听客户端连接

#pragma once

#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <iostream>
#include <cstdio>
#include <memory>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "TcpConnection.hpp"
#include "EventLoopThreadPool.hpp"

#define MAXCONNECTION 20000

class TcpServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::function<void()> ChannelCallback;
    typedef std::function<void(spTcpConnection &)> Callback;
    TcpServer(EventLoop *loop, const int port, const int threadnum = 0);
    ~TcpServer();
    static const std::string ReadMessageHandler;
    static const std::string SendOverHandler;
    static const std::string CloseConnHandler;
    static const std::string ErrorConnHandler;
    // 高层服务向tcpServer注册传递给底层connection->channel的处理函数
    void RegisterHandler(std::string serviceName, const std::string handlerType, Callback *handlerFunc);
    void BindDynamicHandler(spTcpConnection &sptcpconnection); // 动态绑定sptcpconnection的事件处理函数

private:
    std::mutex mutex_;
    Socket tcpServerSocket_;                                                 // 服务监听套接字描述符
    Channel tcpServerChannel_;                                               // 连接Channel实例
    EventLoop *mainLoop_;                                                    // 事件池主逻辑控制实例
    int connCount_;                                                          // 连接计数
    EventLoopThreadPool eventLoopThreadPool;                                 // 多线程事件池
    std::map<int, spTcpConnection> tcpConnList_;                             // 套接字描述符->连接抽象类实例
    std::map<std::string, std::map<std::string, Callback*>> serviceHandlers_;// 不同服务根据服务名及操作名注册的操作函数
    void Setnonblocking(int fd);
    void OnNewConnection();                                  // 处理新连接
    void OnConnectionError();                                // 处理连接错误，关闭套接字
    void RemoveConnection(spTcpConnection &sptcpconnection); // 连接清理，这里应该由EventLoop来执行，投递回主线程删除 OR 多线程加锁删除
    ChannelCallback onNewConnCallback_;
    ChannelCallback onErrConnCallback_;
    Callback onBindConnCallback_;
    Callback onRemoveConnCallback_;

};

const std::string TcpServer::ReadMessageHandler = "ReadMessageHandler";
const std::string TcpServer::SendOverHandler = "SendOverHandler";
const std::string TcpServer::CloseConnHandler = "CloseConnHandler";
const std::string TcpServer::ErrorConnHandler = "ErrorConnHandler";

TcpServer::TcpServer(EventLoop *loop, const int port, const int threadnum)
    : tcpServerSocket_(),
      mainLoop_(loop),
      tcpServerChannel_(),
      connCount_(0),
      eventLoopThreadPool(loop, threadnum),
      onNewConnCallback_(std::bind(&TcpServer::OnNewConnection, this)),
      onErrConnCallback_(std::bind(&TcpServer::OnConnectionError, this)),
      onBindConnCallback_(std::bind(&TcpServer::BindDynamicHandler, this, std::placeholders::_1)),
      onRemoveConnCallback_(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1))
{
    std::cout << "输出测试：TcpServer::TcpServer 创建一个监听端口：" << port << "，io线程数为：" << threadnum << "的TcpServer监听" << std::endl;
    tcpServerSocket_.SetReuseAddr();
    tcpServerSocket_.BindAddress(port);
    tcpServerSocket_.Listen();
    tcpServerSocket_.SetNonblocking();
    tcpServerChannel_.SetFd(tcpServerSocket_.fd()); // TcpServer服务Channel绑定服务套接字tcpServerSocket_
    tcpServerChannel_.SetReadHandle(&onNewConnCallback_);
    tcpServerChannel_.SetErrorHandle(&onErrConnCallback_);
    tcpServerChannel_.SetEvents(EPOLLIN | EPOLLET); // 设置当前连接的监听事件
    std::cout << "输出测试：TcpServer::TcpServer TcpServer服务套接字添加到MainEventLoop的epoll内进行监听，tcpServerSockfd：" << tcpServerSocket_.fd() << std::endl;
    mainLoop_->AddChannelToPoller(&tcpServerChannel_); // 主事件池添加当前Channel为监听对象
}

TcpServer::~TcpServer()
{
}

/*
 * 设置新连接处理函数
 * 可以重复注册同一个操作函数，实际为覆盖注册
 *
 */
void TcpServer::RegisterHandler(std::string serviceName, const std::string handlerType, Callback *handlerFunc)
{
    std::cout << "输出测试：TcpServer::RegisterHandler 服务：" << serviceName << " 开始注册函数，函数名：" << handlerType << std::endl;
    if (serviceHandlers_.end() == serviceHandlers_.find(serviceName))
    {
        // serviceName服务尚未注册过任何操作函数
        std::map<std::string, Callback*> serviceHandlers;
        serviceHandlers_[serviceName] = std::move(serviceHandlers);
    }
    serviceHandlers_[serviceName][handlerType] = handlerFunc;
}

/*
 * 动态绑定sptcpconnection的事件处理函数
 *
 */
void TcpServer::BindDynamicHandler(spTcpConnection &sptcpconnection)
{
    std::cout << "输出测试：TcpServer::BindDynamicHandler " << std::endl;
    HttpRequestContext &httpRequestContext = sptcpconnection->GetReqestBuffer();
    std::string url = httpRequestContext.url;
    bool isDefaultHttpService = false;
    std::string &serviceName = httpRequestContext.serviceName;
    std::string &handlerName = httpRequestContext.handlerName;
    std::string &resourceUrl = httpRequestContext.resourceUrl;
    serviceName.clear();
    handlerName.clear();
    resourceUrl.clear();
    size_t nextFind = url.find('/', 1);
    if (std::string::npos != nextFind)
    {
        serviceName = url.substr(1, nextFind - 1);
        if (serviceHandlers_.end() == serviceHandlers_.find(serviceName))
        {
            // 服务名解析失败，请求的url无法解析为"/服务名/函数名"的格式，默认为网站式请求
            serviceName.clear();
            isDefaultHttpService = true;
        }
        else
        {
            // 服务名解析成功
            url.erase(0, nextFind);
            nextFind = url.find('/', 1); // 可能有，也可能无
            if (std::string::npos != nextFind)
            {
                handlerName = url.substr(1, nextFind - 1);
                resourceUrl = url.substr(nextFind + 1, url.size() - nextFind);
            }
            nextFind = url.find('?', 1); // 可能有，也可能无
            if (std::string::npos == nextFind)
            {
                handlerName = url.substr(1, nextFind - 1);
                resourceUrl = url.substr(nextFind + 1, url.size() - nextFind);
            }
            else
            {
                handlerName = url.substr(1, url.size() - nextFind);
            }
            // 开始解析函数名
            if (serviceHandlers_[serviceName].end() == serviceHandlers_[serviceName].find(handlerName))
            {
                // 函数名解析失败，本次连接所请求的函数未注册到serviceHandlers_
                sptcpconnection->SetBindedHandler(false);
                return;
            }
            else
            {
                // 函数名解析成功，绑定本次请求的处理函数
                sptcpconnection->SetReqHandler(serviceHandlers_[serviceName][handlerName]);
            }
        }
    }
    else
    {
        // 请求的url无法解析为"/服务名/函数名"的格式，默认为网站式请求
        isDefaultHttpService = true;
    }
    if (isDefaultHttpService)
    {
        // 服务名解析失败，使用网站式处理方法
        if (serviceHandlers_.end() == serviceHandlers_.find("HttpService"))
        {
            // TcpServer未注册HttpServer的任一服务处理函数
            sptcpconnection->SetBindedHandler(false);
            return;
        }
        // 默认使用网站资源处理方法
        serviceName = "HttpService";
        handlerName = "HttpProcess";
        sptcpconnection->SetReqHandler(serviceHandlers_[serviceName][handlerName]);
    }
    std::cout << "输出测试：TcpServer::BindDynamicHandler 解析出请求的服务：" << serviceName << "，sockfd：" << sptcpconnection->fd() << std::endl;
    std::cout << "输出测试：TcpServer::BindDynamicHandler 解析出请求的函数：" << handlerName << "，sockfd：" << sptcpconnection->fd() << std::endl;
    sptcpconnection->SetMessaeCallback(serviceHandlers_[serviceName][TcpServer::ReadMessageHandler]);
    sptcpconnection->SetSendCompleteCallback(serviceHandlers_[serviceName][TcpServer::SendOverHandler]);
    sptcpconnection->SetCloseCallback(serviceHandlers_[serviceName][TcpServer::CloseConnHandler]);
    sptcpconnection->SetErrorCallback(serviceHandlers_[serviceName][TcpServer::ErrorConnHandler]);
    sptcpconnection->SetBindedHandler(true);
}

/*
 * 处理新连接
 *
 */
void TcpServer::OnNewConnection()
{
    std::cout << "输出测试：TcpServer::OnNewConnection " << std::endl;
    struct sockaddr_in clientaddr;
    int clientfd;
    while ((clientfd = tcpServerSocket_.Accept(clientaddr)) > 0)
    {
        // 新连接进入处理
        std::cout << "输出测试：TceServer接受来自" << inet_ntoa(clientaddr.sin_addr)
                  << ":" << ntohs(clientaddr.sin_port)
                  << " 的新连接，sockfd：" << clientfd << std::endl;
        if (++connCount_ >= MAXCONNECTION)
        {
            // TODO 连接超量 connCount_需要-1 线程安全？
            close(clientfd);
            continue;
        }
        Setnonblocking(clientfd);
        // 从多线程事件池获取一个事件池索引，该事件池可能是主事件池线程，也可能是事件池工作线程
        // 无论是哪一种，在运行期间都会循环调用loop的poll监听直至服务关闭
        EventLoop *loop = eventLoopThreadPool.GetNextLoop();
        // 创建连接抽象类实例TcpConnection，创建时clientfd已有请求数据待读取
        spTcpConnection sptcpconnection = std::make_shared<TcpConnection>(loop, clientfd, clientaddr);
        sptcpconnection->SetDynamicHandler(&onBindConnCallback_);
        sptcpconnection->SetConnectionCleanUp(&onRemoveConnCallback_);
        {
            // 无名作用域
            std::lock_guard<std::mutex> lock(mutex_);
            tcpConnList_[clientfd] = sptcpconnection;
        }
        sptcpconnection->AddChannelToLoop();
    }
}

/*
 * 处理连接错误，关闭套接字
 *
 */
void TcpServer::OnConnectionError()
{
    std::cout << "输出测试：TcpServer::OnConnectionError TcpServer Received an UNKNOWN EVENT And Will EXIT" << std::endl;
    tcpServerSocket_.Close();
}

/*
 * 连接清理，这里应该由EventLoop来执行，投递回主线程删除 OR 多线程加锁删除
 *
 */
void TcpServer::RemoveConnection(spTcpConnection &sptcpconnection)
{
    std::cout << "输出测试：TcpServer::RemoveConnection TcpServer即将断开与sockfd为" << sptcpconnection->fd() << "的客户端的连接" << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    --connCount_;
    tcpConnList_.erase(sptcpconnection->fd());
}

/*
 * 设置非阻塞IO
 *
 */
void TcpServer::Setnonblocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(fd,GETFL)");
        exit(1);
    }
    if (fcntl(fd, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        perror("fcntl(fd,SETFL,opts)");
        exit(1);
    }
}
