
// 文件资源获取及上传服务
//  与HttpServer的Get/Post与Put方法获取/上传资源文件相似，
//  HttpServer仅需将请求的资源写在url内，以/www目录的相对位置访问资源，
//  而ResourceServer的网络请求url可用于服务定位（服务名、函数名）及资源访问，
//  以/resource目录的相对位置访问资源，
//  header、content内可传递信息，content内传递的信息为json格式数据

#pragma once

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <functional>
#include "Timer.hpp"
#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "ThreadPool.hpp"
#include "TcpConnection.hpp"


class ResourceServer
{
public:
    ResourceServer(EventLoop *loop, const int workThreadNum = 2, ThreadPool *threadPool = NULL, const int loopThreadNum = 0, const int port = 80, TcpServer *shareTcpServer = NULL);
    ~ResourceServer();

private:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    std::string serviceName_;
    std::mutex mutex_;
    int workThreadNum_;
    int loopThreadNum_;
    int tcpServerPort_;         // tcpServer的EPOLL的服务端口
    TcpServer *tcpserver_;      // 基础网络服务TcpServer
    ThreadPool *threadpool_;    // 线程池
    void ResourceProcess(spTcpConnection &sptcpconn);   // 处理请求并响应
    // 处理错误http请求，返回错误描述
    void HttpError(spTcpConnection &sptcpconn, const int err_num, const std::string &short_msg);
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
    // 基于TcpServer设置ResourceServer服务函数，在TcpServer内触发调用ResourceServer的成员函数，类似于信号槽机制
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ReadMessageHandler, std::bind(&ResourceServer::HandleMessage, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::SendOverHandler, std::bind(&ResourceServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::CloseConnHandler, std::bind(&ResourceServer::HandleClose, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ErrorConnHandler, std::bind(&ResourceServer::HandleError, this, std::placeholders::_1));
    if(tcpServerPort_) threadpool_->Start();
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

/*
 * ResourceServer模式处理收到的请求
 * 
 */
void ResourceServer::HandleMessage(spTcpConnection &sptcpconn)
{
    // 修改定时器参数
    sptcpconn->GetTimer()->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    if (false == sptcpconn->GetReqHealthy())
    {
        HttpError(sptcpconn, 400, "Bad request");
        return;
    }
    if (threadpool_->GetThreadNum() > 0)
    {
        // 已开启线程池，设置异步处理标志
        sptcpconn->SetAsyncProcessing(true);
        // 线程池在此添加任务并唤醒一工作线程执行之
        threadpool_->AddTask([&]()
                            {
                                ResourceProcess(sptcpconn);
                                if (!sptcpconn->WillKeepAlive())
                                {
                                    sptcpconn->HandleClose();
                                }
                            });
    }
    else
    {
        // 没有开启线程池
        ResourceProcess(sptcpconn);
        if (!sptcpconn->WillKeepAlive())
        {
            sptcpconn->HandleClose();
        }
    }
}

/*
 * 处理错误http请求，返回错误描述
 * 
 */
void ResourceServer::HttpError(spTcpConnection &sptcpconn, const int err_num, const std::string &short_msg)
{
    std::string &responsecontext = sptcpconn->GetBufferOut();
    if (sptcpconn->GetReq().version.empty())
    {
        responsecontext += "HTTP/1.1 " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    else
    {
        responsecontext += sptcpconn->GetReq().version + " " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    responsecontext += "Server: Qiu Hai's NetServer/0.1\r\n";
    responsecontext += "Content-Type: text/html\r\n";
    responsecontext += "Connection: Keep-Alive\r\n";
    std::string responsebody;
    responsebody += "<html><title>出错了</title>";
    responsebody += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>";
    responsebody += "<style>body{background-color:#f;font-size:14px;}h1{font-size:60px;color:#eeetext-align:center;padding-top:30px;font-weight:normal;}</style>";
    responsebody += "<body bgcolor=\"ffffff\"><h1>";
    responsebody += std::to_string(err_num) + " " + short_msg;
    responsebody += "</h1><hr><em> Qiu Hai's NetServer</em>\n</body></html>";
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
    responsecontext += "\r\n";
    responsecontext.append(responsebody, 0, responsebody.length());
    sptcpconn->SendBufferOut();
}

/*
 * 处理请求并响应
 * 
 */
void ResourceServer::ResourceProcess(spTcpConnection &sptcpconn)
{
    ;
}

/*
 * ResourceServer模式数据处理发送客户端完毕
 * 
 */
void ResourceServer::HandleSendComplete(spTcpConnection &sptcpconn)
{
    ;
}

/*
 * ResourceServer模式处理连接断开
 * 
 */
void ResourceServer::HandleClose(spTcpConnection &sptcpconn)
{
    ;
}

/*
 * ResourceServer模式处理连接出错
 * 
 */
void ResourceServer::HandleError(spTcpConnection &sptcpconn)
{
    ;
}


