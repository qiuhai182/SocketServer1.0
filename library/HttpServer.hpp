
#pragma once

#include <string>
#include <mutex>
#include <map>
#include <iostream>
#include <functional>
#include <memory>
#include "TimerManager.hpp"
#include "EventLoop.hpp"
#include "TcpServer.hpp"
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
    // bugfix:声明顺序调整，map、mutex放到最后析构
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
 * 
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
 * 
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
    sptimer->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    std::string responsecontext;
    if (threadpool_.GetThreadNum() > 0)
    {
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
        // 没有开启业务线程池，业务计算直接在IO线程执行
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
            // 短连接，关闭连接
            // sptcpconn->HandleClose();
        }
    }
}

/*
 * 
 * 
 */
void HttpServer::HandleSendComplete(const spTcpConnection &sptcpconn)
{
}

/*
 * 
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
 * 
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
 * 
 * 
 */
void HttpServer::Start()
{
    tcpserver_.Start();
}
