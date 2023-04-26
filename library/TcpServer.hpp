
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
    typedef std::function<void(spTcpConnection &)> Callback;
    TcpServer(EventLoop *loop, const int port, const int threadnum = 0, bool coverAllService = false);
    ~TcpServer();
    static const std::string ReadMessageHandler;
    static const std::string SendOverHandler;
    static const std::string CloseConnHandler;
    static const std::string ErrorConnHandler;
    static const std::string HttpServiceName;
    static const std::string HttpHandler;
    static const std::string CoverServiceName;
    static const std::string CoverHandler;
    // 高层服务向tcpServer注册传递给底层connection->channel的处理函数，
    // coverAllService_参数默认为false，若为true则此TcpServer仅提供一种服务的各个处理函数，其他服务在此处无法绑定
    void RegisterHandler(std::string serviceName, const std::string handlerType, 
                            const Callback &handlerFunc, bool coverAllService = false);
    void BindDynamicHandler(spTcpConnection &sptcpconnection); // 动态绑定sptcpconnection的事件处理函数

private:
    std::mutex mutex_;
    Socket tcpServerSocket_;                    // 服务监听套接字描述符
    Channel tcpServerChannel_;                  // TcpSeve内置连接Channel实例，用于监听客户端连接事件
    EventLoop *mainLoop_;                       // 事件池主逻辑控制实例
    int connCount_;                             // 连接计数
    EventLoopThreadPool eventLoopThreadPool;    // 多线程事件池
    bool coverAllService_;                      // 是否启动覆盖服务绑定模式，该模式下本TcpServer仅可绑定一类服务
    std::map<int, spTcpConnection> tcpConnList_;                             // 套接字描述符->连接抽象类实例
    std::map<std::string, std::map<std::string, Callback>> serviceHandlers_; // 不同服务根据服务名及操作名注册的操作函数
    void Setnonblocking(int fd);
    void OnNewConnection();                                  // 处理新连接
    void OnConnectionError();                                // 处理连接错误，关闭套接字
    void RemoveConnection(spTcpConnection &sptcpconnection); // 连接清理，这里应该由EventLoop来执行，投递回主线程删除 OR 多线程加锁删除

};

const std::string TcpServer::ReadMessageHandler = "ReadMessageHandler";
const std::string TcpServer::SendOverHandler = "SendOverHandler";
const std::string TcpServer::CloseConnHandler = "CloseConnHandler";
const std::string TcpServer::ErrorConnHandler = "ErrorConnHandler";
const std::string TcpServer::HttpServiceName = "HttpService";
const std::string TcpServer::HttpHandler = "HttpHandler";
const std::string TcpServer::CoverServiceName = "CoverService";
const std::string TcpServer::CoverHandler = "CoverHandler";

TcpServer::TcpServer(EventLoop *loop, const int port, const int threadnum, bool coverAllService)
    : tcpServerSocket_(),
      mainLoop_(loop),
      tcpServerChannel_(),
      connCount_(0),
      eventLoopThreadPool(loop, threadnum),
      coverAllService_(coverAllService)
{
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
    LOG(LoggerLevel::INFO, "创建一个监听端口：%d，io线程数：%d，服务sockfd：%d\n", port, threadnum, tcpServerSocket_.fd());
    std::cout << "TcpServer::TcpServer 创建一个监听端口为：" << port << "、io线程数为：" << threadnum << " 的TcpServer监听" << std::endl;
    tcpServerSocket_.SetReuseAddr();
    tcpServerSocket_.BindAddress(port);
    tcpServerSocket_.Listen();
    tcpServerSocket_.SetNonblocking();
    // 为内置tcpServerChannel_绑定连接处理函数，用于处理客户端连接事件
    tcpServerChannel_.SetFd(tcpServerSocket_.fd()); // TcpServer服务Channel绑定服务套接字tcpServerSocket_
    tcpServerChannel_.SetReadHandle(std::bind(&TcpServer::OnNewConnection, this));
    tcpServerChannel_.SetErrorHandle(std::bind(&TcpServer::OnConnectionError, this));
    tcpServerChannel_.SetEvents(EPOLLIN | EPOLLET); // 设置当前连接的监听事件
    LOG(LoggerLevel::INFO, "服务套接字添加到MainEventLoop的epoll内进行监听，服务sockfd：%d\n", tcpServerSocket_.fd());
    std::cout << "TcpServer::TcpServer 服务套接字添加到MainEventLoop的epoll内进行监听，sockfd：" << tcpServerSocket_.fd() << std::endl;
    mainLoop_->AddChannelToPoller(&tcpServerChannel_); // 主事件池添加当前内置Channel为监听对象，监听客户端连接事件
}

TcpServer::~TcpServer()
{
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
}

/*
 * 设置新连接处理函数
 * 可以重复注册同一个操作函数，实际为覆盖注册
 * 一旦coverAllService_标志被置为true，后续所有的函数注册都是对默认服务的函数进行注册或覆盖
 * coverAllService_标志可在TcpServe初始化时指定为true，也可在此处置为true
 * 不论在哪里置为true，若在未定义默认服务时有请求接入则会不安全
 * 若在此函数置为true，在置为true且尚未定义默认服务函数时有请求接入也会不安全
 * 后续注册函数时也必须填入coverAllService=true参数，否则不予注册
 *
 */
void TcpServer::RegisterHandler(std::string serviceName, const std::string handlerType, const Callback &handlerFunc, bool coverAllService)
{
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
    LOG(LoggerLevel::INFO, "高级服务：%s开始注册函数，函数名：%s，服务sockfd：%d\n", serviceName.data(), handlerType.c_str(), tcpServerSocket_.fd());
    std::cout << "TcpServer::RegisterHandler 高级服务：" << serviceName << " 开始注册函数，函数名：" << handlerType << std::endl;
    if(coverAllService_ && !coverAllService) return;
    if (coverAllService)
    {
        coverAllService_ = coverAllService;
        serviceName = TcpServer::CoverServiceName;
    }
    if (serviceHandlers_.end() == serviceHandlers_.find(serviceName))
    {
        // serviceName服务尚未注册过任何操作函数
        std::map<std::string, Callback> serviceHandlers;
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
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
    HttpRequestContext &httpRequestContext = sptcpconnection->GetReqestBuffer();
    std::string url = httpRequestContext.url;
    bool isDefaultHttpService = false;
    std::string serviceName, handlerName, resourceUrl;
    LOG(LoggerLevel::INFO, "开始解析服务url：%s，服务sockfd：%d\n", url.data(), tcpServerSocket_.fd());
    // std::cout << "TcpServer::BindDynamicHandler 开始解析服务url：" << url << std::endl;
    // 解析请求的服务，绑定注册的各类服务提供函数
    size_t nextFind = url.find('/', 1);
    if (std::string::npos != nextFind)
    {
        // 解析到字符'/'
        serviceName = url.substr(1, nextFind - 1);
        httpRequestContext.serviceName = serviceName;
        if (serviceHandlers_.end() == serviceHandlers_.find(serviceName))
        {
            // 服务名映射的服务不存在或尚未注册服务在此端口，默认为网站式请求
            LOG(LoggerLevel::ERROR, "服务名映射的服务不存在或尚未注册服务在此端口，默认为网站式请求，服务sockfd：%d\n", tcpServerSocket_.fd());
            // std::cout << "TcpServer::BindDynamicHandler 服务名映射的服务不存在或尚未注册服务在此端口，默认为网站式请求，sockfd：" << sptcpconnection->fd() << std::endl;
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
                LOG(LoggerLevel::ERROR, "解析服务失败，服务sockfd：%d\n", tcpServerSocket_.fd());
                // std::cout << "TcpServer::BindDynamicHandler 解析服务失败，sockfd：" << sptcpconnection->fd() << std::endl;
                if (!coverAllService_)
                {
                    sptcpconnection->SetBindedHandler(false);
                    return;
                }
            }
        }
    }
    else
    {
        // 请求的url无法解析为"/服务名/函数名"的格式，默认为网站式请求
        // std::cout << "TcpServer::BindDynamicHandler 请求url的服务指定格式不为“/服务名/函数名”，sockfd：" << sptcpconnection->fd() << std::endl;
        isDefaultHttpService = true;
    }
    if (isDefaultHttpService)
    {
        // 服务名解析失败，默认绑定网站服务函数
        if (serviceHandlers_.end() == serviceHandlers_.find("HttpService"))
        {
            // TcpServer未注册HttpServer的任一服务处理函数，绑定失败并返回
            // std::cout << "输出测试：TcpServer::BindDynamicHandler 本端口找不到网站服务，sockfd：" << sptcpconnection->fd() << std::endl; 
            if (!coverAllService_)
            {
                sptcpconnection->SetBindedHandler(false);
                return;
            }
        }
        // 默认使用网站资源处理方法
        serviceName = TcpServer::HttpServiceName;
        handlerName = TcpServer::HttpHandler;
    }
    // 解析出的服务名、函数名存入sptcpconnection的httpRequestContext
    httpRequestContext.serviceName = serviceName;
    httpRequestContext.handlerName = handlerName;
    httpRequestContext.resourceUrl = resourceUrl;
    if (coverAllService_)
    {
        // 启动覆盖服务绑定模式，此TcpServe仅提供注册的单一服务
        if (serviceHandlers_.end() == serviceHandlers_.find(TcpServer::CoverServiceName))
        {
            // TcpServer未注册TcpServer::CoverServiceName的任一服务处理函数，绑定失败并返回
            LOG(LoggerLevel::ERROR, "启用单一覆盖服务模式但尚未注册服务函数，服务sockfd：%d\n", tcpServerSocket_.fd());
            // std::cout << "TcpServer::BindDynamicHandler 启用单一覆盖服务模式但尚未注册服务函数，sockfd：" << sptcpconnection->fd() << std::endl;
            sptcpconnection->SetBindedHandler(false);
            return;
        }
        serviceName = TcpServer::CoverServiceName;
        handlerName = TcpServer::CoverHandler;
    }
    LOG(LoggerLevel::INFO, "解析出请求的函数：%s，服务sockfd：%d\n", handlerName.c_str(), tcpServerSocket_.fd());
    std::cout << "TcpServer::BindDynamicHandler 解析出请求的函数：" << handlerName << "，sockfd：" << sptcpconnection->fd() << std::endl;
    sptcpconnection->SetMessaeCallback(serviceHandlers_[serviceName][TcpServer::ReadMessageHandler]);
    sptcpconnection->SetSendCompleteCallback(serviceHandlers_[serviceName][TcpServer::SendOverHandler]);
    sptcpconnection->SetCloseCallback(serviceHandlers_[serviceName][TcpServer::CloseConnHandler]);
    sptcpconnection->SetErrorCallback(serviceHandlers_[serviceName][TcpServer::ErrorConnHandler]);
    sptcpconnection->SetReqHandler(serviceHandlers_[serviceName][handlerName]);
    sptcpconnection->SetBindedHandler(true);
}

/*
 * 接受并处理一个新连接
 * 该函数一般作为注册绑定回调函数
 * 传入到TcpServer内置Channel对象内待新连接到来调用
 *
 */
void TcpServer::OnNewConnection()
{
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
    struct sockaddr_in clientaddr;
    int clientfd;
    while ((clientfd = tcpServerSocket_.Accept(clientaddr)) > 0)
    {
        // 新连接进入处理
        LOG(LoggerLevel::INFO, "TceServer接受来自%s:%d的新连接sockfd:%d,，服务sockfd：%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), clientfd, tcpServerSocket_.fd());
        std::cout << "TcpServer::OnNewConnection TceServer接受来自" << inet_ntoa(clientaddr.sin_addr)
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
        // 创建连接抽象类实例TcpConnection
        // 该对象是客户端连接的抽象表示，能在其中对连接进行业务逻辑操作
        spTcpConnection sptcpconnection = std::make_shared<TcpConnection>(loop, clientfd, clientaddr);
        sptcpconnection->SetDynamicHandler(std::bind(&TcpServer::BindDynamicHandler, this, std::placeholders::_1));
        sptcpconnection->SetConnectionCleanUp(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
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
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
    LOG(LoggerLevel::ERROR, "TcpServer接收到未知event，将关闭该服务TcpServe，服务sockfd：%d\n", tcpServerSocket_.fd());
    std::cout << "TcpServer::OnConnectionError TcpServer接收到未知event，将关闭该服务TcpServe" << std::endl;
    tcpServerSocket_.Close();
}

/*
 * 连接清理，这里应该由EventLoop来执行，投递回主线程删除 OR 多线程加锁删除
 *
 */
void TcpServer::RemoveConnection(spTcpConnection &sptcpconnection)
{
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
    LOG(LoggerLevel::INFO, "TcpServer即将断开与sockfd为：%d的客户端连接，服务sockfd：%d\n", sptcpconnection->fd(), tcpServerSocket_.fd());
    std::cout << "TcpServer::RemoveConnection TcpServer即将断开与sockfd为" << sptcpconnection->fd() << "的客户端的连接" << std::endl;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        --connCount_;
        tcpConnList_.erase(sptcpconnection->fd());
    }
    std::cout << "TcpServer::RemoveConnection sockfd为" << sptcpconnection->fd() << "的客户端的连接use_count为：" << sptcpconnection.use_count() << std::endl;
}

/*
 * 设置非阻塞IO
 *
 */
void TcpServer::Setnonblocking(int fd)
{
    LOG(LoggerLevel::INFO, "%s，服务sockfd：%d\n", "函数触发", tcpServerSocket_.fd());
    int opts = fcntl(fd, F_GETFL);
    if (opts < 0)
    {
        LOG(LoggerLevel::ERROR, "获取socket为：%d的文件描述符失败，退出，服务sockfd：%d\n", fd, tcpServerSocket_.fd());
        std::cout << "TcpServer::SetNonblocking 获取socket的描述符失败，退出" << std::endl;
        perror("获取socket的描述符失败（fcntl(fd,GETFL)）");
        exit(1);
    }
    if (fcntl(fd, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        LOG(LoggerLevel::ERROR, "更改socket为：%d的文件描述符失败，退出，服务sockfd：%d\n", fd, tcpServerSocket_.fd());
        std::cout << "TcpServer::SetNonblocking 更改socket的描述符失败，退出" << std::endl;
        perror("更改socket的描述符失败（fcntl(fd,SETFL,opts)）");
        exit(1);
    }
}
