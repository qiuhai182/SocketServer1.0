
// tcpServer类，实现基于socket的网络服务，是其他一切网络服务的基础

#pragma once

#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
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
    typedef std::function<void(const spTcpConnection &, std::string &)> MessageCallback;
    typedef std::function<void(const spTcpConnection &)> Callback;
    TcpServer(EventLoop *loop, const int port, const int threadnum = 0);
    ~TcpServer();
    void Start();
    void SetNewConnCallback(const Callback &cb);
    void SetMessageCallback(const MessageCallback &cb);
    void SetSendCompleteCallback(const Callback &cb);
    void SetCloseCallback(const Callback &cb);
    void SetErrorCallback(const Callback &cb);

private:
    Socket tcpServerSocket_;
    EventLoop *loop_;
    Channel tcpServerChannel_;
    int connCount_;
    std::mutex mutex_;
    EventLoopThreadPool eventLoopThreadPool;
    std::map<int, std::shared_ptr<TcpConnection>> tcpConnList_;
    Callback newConnectionCallback_;
    MessageCallback messageCallback_;
    Callback sendCompleteCallback_;
    Callback closeCallback_;
    Callback errorCallback_;
    void OnNewConnection();
    void OnConnectionError();
    void RemoveConnection(const std::shared_ptr<TcpConnection> sptcpconnection);
};

TcpServer::TcpServer(EventLoop *loop, const int port, const int threadnum)
    : tcpServerSocket_(),
      loop_(loop),
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
 * 启动tcp服务
 * 
 */
void TcpServer::Start()
{
    eventLoopThreadPool.Start();
    tcpServerChannel_.SetEvents(EPOLLIN | EPOLLET);
    loop_->AddChannelToPoller(&tcpServerChannel_);
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
 * 处理新连接，调用绑定的newConnectionCallback_函数
 * 
 */
void TcpServer::OnNewConnection()
{
    struct sockaddr_in clientaddr;
    int clientfd;
    while ((clientfd = tcpServerSocket_.Accept(clientaddr)) > 0)
    {
        std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr)
                  << ":" << ntohs(clientaddr.sin_port) << std::endl;
        if (++connCount_ >= MAXCONNECTION)
        {
            close(clientfd);
            continue;
        }
        Setnonblocking(clientfd);
        EventLoop *loop = eventLoopThreadPool.GetNextLoop();
        std::shared_ptr<TcpConnection> sptcpconnection = std::make_shared<TcpConnection>(loop, clientfd, clientaddr);
        sptcpconnection->SetMessaeCallback(messageCallback_);
        sptcpconnection->SetSendCompleteCallback(sendCompleteCallback_);
        sptcpconnection->SetCloseCallback(closeCallback_);
        sptcpconnection->SetErrorCallback(errorCallback_);
        sptcpconnection->SetConnectionCleanUp(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tcpConnList_[clientfd] = sptcpconnection;
        }
        newConnectionCallback_(sptcpconnection);
        // Bug，应该把事件添加的操作放到最后,否则bug segement fault,导致HandleMessage中的phttpsession==NULL
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
