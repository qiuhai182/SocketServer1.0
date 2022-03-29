
// tcpServer类：
//  实现基于socket的网络服务
//  是其他一切网络服务的基础服务提供类

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

void Setnonblocking(int fd);

class TcpServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::function<void(const spTcpConnection &)> MessageCallback;    // 信息处理函数
    typedef std::function<void(const spTcpConnection &)> Callback;
    TcpServer(EventLoop *loop, const int port, const int threadnum = 0);
    ~TcpServer();
    void Start();   // 创建所需的事件池子线程，添加tcp服务Channel实例为监听对象
    void SetNewConnCallback(const Callback &cb);            // 设置新连接处理函数
    void SetMessageCallback(const MessageCallback &cb);     // 设置消息处理函数
    void SetSendCompleteCallback(const Callback &cb);       // 设置数据发送完毕处理函数
    void SetCloseCallback(const Callback &cb);              // 设置连接关闭处理函数
    void SetErrorCallback(const Callback &cb);              // 设置出错处理函数

private:
    std::mutex mutex_;
    Socket tcpServerSocket_;        // 服务监听套接字描述符
    Channel tcpServerChannel_;      // 连接Channel实例
    EventLoop *mainLoop_;           // 事件池主逻辑控制实例
    int connCount_;                 // 连接计数
    EventLoopThreadPool eventLoopThreadPool;    // 多线程事件池
    std::map<int, std::shared_ptr<TcpConnection>> tcpConnList_; // 套接字描述符->连接抽象类实例
    Callback newConnectionCallback_;    // 新连接处理回调函数
    MessageCallback messageCallback_;   // 消息处理回调函数
    Callback sendCompleteCallback_;     // 数据发送完毕处理回调函数
    Callback closeCallback_;            // 连接关闭处理回调函数
    Callback errorCallback_;            // 出错处理回调函数
    void OnNewConnection();             // 处理新连接，调用绑定的newConnectionCallback_回调函数
    void OnConnectionError();           // 处理连接错误，关闭套接字
    void RemoveConnection(const std::shared_ptr<TcpConnection> sptcpconnection);    // 连接清理，这里应该由EventLoop来执行，投递回主线程删除 OR 多线程加锁删除

};

TcpServer::TcpServer(EventLoop *loop, const int port, const int threadnum)
    : tcpServerSocket_(),
      mainLoop_(loop),
      tcpServerChannel_(),
      connCount_(0),
      eventLoopThreadPool(loop, threadnum)
{
    tcpServerSocket_.SetReuseAddr();
    tcpServerSocket_.BindAddress(port);
    tcpServerSocket_.Listen();
    tcpServerSocket_.SetNonblocking();
    tcpServerChannel_.SetFd(tcpServerSocket_.fd());
    tcpServerChannel_.SetReadHandle(std::bind(&TcpServer::OnNewConnection, this));
    tcpServerChannel_.SetErrorHandle(std::bind(&TcpServer::OnConnectionError, this));
}

TcpServer::~TcpServer()
{
}

/*
 * 创建所需的事件池子线程并启动
 * 添加tcp服务Channel实例为监听对象
 * 
 */
void TcpServer::Start()
{
    eventLoopThreadPool.Start();    // 创建所需的所有事件池子线程实例
    tcpServerChannel_.SetEvents(EPOLLIN | EPOLLET);     // 设置当前连接的监听事件
    mainLoop_->AddChannelToPoller(&tcpServerChannel_);  // 主事件池添加当前Channel为监听对象
}

/*
 * 设置新连接处理函数
 * 
 */
void TcpServer::SetNewConnCallback(const Callback &cb)
{
    newConnectionCallback_ = cb;
}

/*
 * 设置消息处理函数
 * 
 */
void TcpServer::SetMessageCallback(const MessageCallback &cb)
{
    messageCallback_ = cb;
}

/*
 * 设置数据发送完毕处理函数
 * 
 */
void TcpServer::SetSendCompleteCallback(const Callback &cb)
{
    sendCompleteCallback_ = cb;
}

/*
 * 设置连接关闭处理函数
 * 
 */
void TcpServer::SetCloseCallback(const Callback &cb)
{
    closeCallback_ = cb;
}

/*
 * 设置出错处理函数
 * 
 */
void TcpServer::SetErrorCallback(const Callback &cb)
{
    errorCallback_ = cb;
}

/*
 * 处理新连接，再调用绑定的newConnectionCallback_函数
 * 
 */
void TcpServer::OnNewConnection()
{
    struct sockaddr_in clientaddr;
    int clientfd;
    while ((clientfd = tcpServerSocket_.Accept(clientaddr)) > 0)
    {
        // 新连接进入处理
        std::cout << "输出测试： TceServer->TcpServerChannel handle new connection from IP:" << inet_ntoa(clientaddr.sin_addr)
                  << ":" << ntohs(clientaddr.sin_port)
                  << " 连接socket：" << clientfd << std::endl;
        if (++connCount_ >= MAXCONNECTION)
        {
            // 连接超量 TODO connCount_需要-1，检查是否做了
            close(clientfd);
            continue;
        }
        Setnonblocking(clientfd);
        // 从多线程事件池获取一个事件池索引，该事件池可能是主事件池
        // 也可能是事件池工作线程
        // 无论是哪一种，在运行期间都会循环调用loop的poll监听直至服务关闭
        EventLoop *loop = eventLoopThreadPool.GetNextLoop();
        // 创建连接抽象类实例TcpConnection
        std::shared_ptr<TcpConnection> sptcpconnection = std::make_shared<TcpConnection>(loop, clientfd, clientaddr);
        // 基于TcpConnection设置TcpServer服务函数，在TcpConnection内触发调用TcpServer的成员函数，类似于信号槽机制
        sptcpconnection->SetMessaeCallback(messageCallback_);
        sptcpconnection->SetSendCompleteCallback(sendCompleteCallback_);
        sptcpconnection->SetCloseCallback(closeCallback_);
        sptcpconnection->SetErrorCallback(errorCallback_);
        sptcpconnection->SetConnectionCleanUp(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
        {
            // 无名作用域
            std::lock_guard<std::mutex> lock(mutex_);
            tcpConnList_[clientfd] = sptcpconnection;
        }
        newConnectionCallback_(sptcpconnection); // 调用动态绑定的newConnectionCallback_函数
        // TODO Bug，应该把事件添加的操作放到最后,否则bug segement fault,导致HandleMessage中的phttpsession==NULL
        // 总之就是做好一切准备工作再添加事件到epoll！！！
        sptcpconnection->AddChannelToLoop();
    }
}

/*
 * 处理连接错误，关闭套接字
 * 
 */
void TcpServer::OnConnectionError()
{
    std::cout << "UNKNOWN EVENT" << std::endl;
    tcpServerSocket_.Close();
}

/*
 * 连接清理，这里应该由EventLoop来执行，投递回主线程删除 OR 多线程加锁删除
 * 
 */
void TcpServer::RemoveConnection(std::shared_ptr<TcpConnection> sptcpconnection)
{
    std::lock_guard<std::mutex> lock(mutex_);
    --connCount_;
    tcpConnList_.erase(sptcpconnection->fd());
}

/*
 * 设置非阻塞IO
 * 
 */
void Setnonblocking(int fd)
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
