
//简单回发服务器

#pragma once

#include <iostream>
#include <functional>
#include <string>
#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "TcpConnection.hpp"
#include "Timer.hpp"

class EchoServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::shared_ptr<Timer> spTimer;
    EchoServer(EventLoop *loop, const int port, const int threadnum);
    ~EchoServer();
    void Start();

private:
    void HandleNewConnection(const spTcpConnection &sptcpconn);
    void HandleMessage(const spTcpConnection &sptcpconn, std::string &s);
    void HandleSendComplete(const spTcpConnection &sptcpconn);
    void HandleClose(const spTcpConnection &sptcpconn);
    void HandleError(const spTcpConnection &sptcpconn);
    TcpServer tcpserver_;
};

EchoServer::EchoServer(EventLoop *loop, const int port, const int threadnum)
    : tcpserver_(loop, port, threadnum)
{
    tcpserver_.SetNewConnCallback(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.SetMessageCallback(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.SetSendCompleteCallback(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_.SetCloseCallback(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.SetErrorCallback(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
}

EchoServer::~EchoServer()
{
}

void EchoServer::Start()
{
    tcpserver_.Start();
}

/*
 * 
 * 
 */
void EchoServer::HandleMessage(const spTcpConnection &sptcpconn, std::string &s)
{
    //std::string msg("reply msg:");
    //msg += s;
    //s.clear();
    std::string msg;
    msg.swap(s);
    msg.insert(0, "reply msg:");
    sptcpconn->Send(msg);
}

/*
 * 
 * 
 */
void EchoServer::HandleNewConnection(const spTcpConnection &sptcpconn)
{
    std::cout << "New Connection Come in" << std::endl;
}

/*
 * 
 * 
 */
void EchoServer::HandleSendComplete(const spTcpConnection &sptcpconn)
{
    std::cout << "Message send complete" << std::endl;
}

/*
 * 
 * 
 */
void EchoServer::HandleClose(const spTcpConnection &sptcpconn)
{
    std::cout << "EchoServer conn close" << std::endl;
}

/*
 * 
 * 
 */
void EchoServer::HandleError(const spTcpConnection &sptcpconn)
{
    std::cout << "EchoServer error" << std::endl;
}
