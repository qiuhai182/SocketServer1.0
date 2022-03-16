
// http网络服务类

#pragma once

#include <string>
#include <mutex>
#include <map>
#include <iostream>
#include <functional>
#include <memory>
#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "TimerManager.hpp"
#include "TcpConnection.hpp"
#include "HttpSession.hpp"
#include "ThreadPool.hpp"
#include "Timer.hpp"

class HttpServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::shared_ptr<Timer> spTimer;
    HttpServer(EventLoop *loop, const int port, const int iothreadnum, const int workerthreadnum);
    ~HttpServer();
    void Start();

private:
    void HandleNewConnection(const spTcpConnection &sptcpconn);
    void HandleMessage(const spTcpConnection &sptcpconn, std::string &msg);
    void HandleSendComplete(const spTcpConnection &sptcpconn);
    void HandleClose(const spTcpConnection &sptcpconn);
    void HandleError(const spTcpConnection &sptcpconn);
    std::map<spTcpConnection, std::shared_ptr<HttpSession>> httpSessionnList_;
    std::map<spTcpConnection, spTimer> timerList_;
    std::mutex mutex_;
    TcpServer tcpserver_;
    ThreadPool threadpool_;

};

HttpServer::HttpServer(EventLoop *loop, const int port, const int iothreadnum, const int workerthreadnum)
    : tcpserver_(loop, port, iothreadnum),
      threadpool_(workerthreadnum)
{
    tcpserver_.SetNewConnCallback(std::bind(&HttpServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.SetMessageCallback(std::bind(&HttpServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.SetSendCompleteCallback(std::bind(&HttpServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_.SetCloseCallback(std::bind(&HttpServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.SetErrorCallback(std::bind(&HttpServer::HandleError, this, std::placeholders::_1));
    threadpool_.Start();
    TimerManager::GetTimerManagerInstance()->Start();
}

HttpServer::~HttpServer()
{
}

/*
 * 处理新连接
 * 
 */
void HttpServer::HandleNewConnection(const spTcpConnection &sptcpconn)
{
    std::shared_ptr<HttpSession> sphttpsession = std::make_shared<HttpSession>();
    spTimer sptimer = std::make_shared<Timer>(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    sptimer->Start();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        httpSessionnList_[sptcpconn] = sphttpsession;
        timerList_[sptcpconn] = sptimer;
    }
}

/*
 * 处理收到的请求
 * 
 */
void HttpServer::HandleMessage(const spTcpConnection &sptcpconn, std::string &msg)
{
    std::shared_ptr<HttpSession> sphttpsession;
    spTimer sptimer;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sphttpsession = httpSessionnList_[sptcpconn];
        sptimer = timerList_[sptcpconn];
    }
    // 定时关闭连接
    sptimer->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    std::string responsecontext;
    if (threadpool_.GetThreadNum() > 0)
    {
        // 已开启线程池，解析http请求，设置异步处理标志，线程池taskQueue_添加任务
        HttpRequestContext httprequestcontext;
        bool result = sphttpsession->ParseHttpRequest(msg, httprequestcontext);
        if (result == false)
        {
            sphttpsession->HttpError(400, "Bad request", httprequestcontext, responsecontext);
            sptcpconn->Send(responsecontext);
            return;
        }
        sptcpconn->SetAsyncProcessing(true);
        threadpool_.AddTask([=]()
                            {
                                std::string responsecontext;
                                sphttpsession->HttpProcess(httprequestcontext, responsecontext);
                                sptcpconn->Send(responsecontext);
                                if (!sphttpsession->KeepAlive())
                                {
                                    // 短连接，关闭连接
                                    // sptcpconn->HandleClose();
                                }
                            });
    }
    else
    {
        // 没有开启线程池
        HttpRequestContext httprequestcontext;
        bool result = sphttpsession->ParseHttpRequest(msg, httprequestcontext);
        if (result == false)
        {
            sphttpsession->HttpError(400, "Bad request", httprequestcontext, responsecontext); // 请求报文解析错误，报400
            sptcpconn->Send(responsecontext);
            return;
        }
        sphttpsession->HttpProcess(httprequestcontext, responsecontext);
        sptcpconn->Send(responsecontext);
        if (!sphttpsession->KeepAlive())
        {
            // sptcpconn->HandleClose();
        }
    }
}

/*
 * 数据发送客户端完毕
 * 
 */
void HttpServer::HandleSendComplete(const spTcpConnection &sptcpconn)
{
}

/*
 * 连接断开
 * 
 */
void HttpServer::HandleClose(const spTcpConnection &sptcpconn)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        httpSessionnList_.erase(sptcpconn);
        timerList_.erase(sptcpconn);
    }
}

/*
 * 连接出错
 * 
 */
void HttpServer::HandleError(const spTcpConnection &sptcpconn)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        httpSessionnList_.erase(sptcpconn);
        timerList_.erase(sptcpconn);
    }
}

/*
 * 启动http服务
 * 
 */
void HttpServer::Start()
{
    tcpserver_.Start();
}
