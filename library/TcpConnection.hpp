
// TcpConnection类：
//  客户端连接的抽象表示，每一个客户端连接对应一个TcpConnection
//  每一个TcpConnection内部生成一个Channel实例用于tcp数据交互

#pragma once

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <memory>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Timer.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "TypeIdentify.hpp"

#define BUFSIZE 4096

// http请求信息结构
typedef struct _HttpRequestContext
{
    std::string method;      // http方法（get、post等）
    std::string url;         // http url
    std::string serviceName; // url解析出请求的服务名
    std::string handlerName; // url解析出请求的服务的处理函数
    std::string resourceUrl; // url的"/服务名/函数名"后的部分
    std::string version;     // http version
    std::map<std::string, std::string> header;
    std::string body;
} HttpRequestContext;

// http响应信息结构
typedef struct _HttpResponseContext
{
    std::string version;
    std::string statecode;
    std::string statemsg;
    std::map<std::string, std::string> header;
    std::string body;
} HttpResponseContext;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{ // 允许安全使用shared_ptr
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;  // 指向TcpConnection的智能指针
    typedef std::function<void(spTcpConnection &)> Callback; // 回调函数
    TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr);
    ~TcpConnection();
    int fd() const { return fd_; }               // 获取套接字描述符
    EventLoop *GetLoop() const { return loop_; } // 获取事件池指针
    int recvn(int fd, std::string &recvMsg);     // 从客户端fd接收数据
    int sendn(int fd, std::string &sendMsg);     // 发送数据到客户端fd
    void Send(const std::string &s);             // 发送信息函数，指定EventLoop执行
    void Send(const char *s, int length = 0);    // 发送信息函数，指定EventLoop执行
    void SendBufferOut();                        // 发送信息函数，仅发送bufferOut_存储的内容，指定EventLoop执行
    bool ParseHttpRequest();                     // 解析http请求信息
    bool GetReqHealthy();                        // 获取连接请求解析结果状态
    void SendInLoop();                           // 发送信息函数，由EventLoop执行
    void AddChannelToLoop();                     // EventLoop添加监听Channel
    void Shutdown();                             // 关闭当前连接，指定EventLoop执行HandleClose函数
    void HandleRead();                           // 由TcpConnection的Channel调用，接收客户端发送的数据，再调用绑定的messageCallback_函数
    void HandleWrite();                          // 由TcpConnection的Channel调用，向客户端发送数据，再调用绑定的sendcompleteCallback_函数
    void HandleError();                          // 由TcpConnection的Channel调用，处理连接错误，再调用绑定的errorCallback_函数及HandleClose函数
    void HandleClose();                          // 由TcpConnection的Channel调用，处理客户端连接关闭，再调用绑定的closeCallback_函数与connectioncleanup_函数
    std::string &GetBufferIn();                  // 获取接收缓冲区的指针
    std::string &GetBufferOut();                 // 获取发送缓冲区的指针
    int GetReceiveLength();                      // 获取接收到的数据的长度
    int GetSendLength();                         // 获取待发送数据的长度
    Timer *GetTimer();                           // 获取定时器指针
    void StartTimer();                           // 启动定时器
    bool IsDisconnected();                       // 判断连接是否已关闭
    bool WillKeepAlive();                        // 获取长连接标志
    void SetKeepAlive(bool keepalive);           // 设置长连接标志
    HttpRequestContext &GetReqestBuffer();       // 获取请求解析结构体的引用
    HttpResponseContext &GetResonseBuffer();     // 获取响应解析结构体的引用
    Callback GetMessageCallback();               // 设置连接处理函数
    Callback GetSendCompleteCallback();          // 设置数据发送完毕处理函数
    Callback GetCloseCallback();                 // 设置关闭处理函数
    Callback GetErrorCallback();                 // 设置出错处理函数
    Callback GetConnectionCleanUp();             // 设置连接清理函数
    // 处理错误http请求，返回错误描述
    void HttpError(const int err_num, const std::string &short_msg);
    void SetMessaeCallback(const Callback &cb);          // 设置连接处理函数
    void SetSendCompleteCallback(const Callback &cb);    // 设置数据发送完毕处理函数
    void SetCloseCallback(const Callback &cb);           // 设置关闭处理函数
    void SetErrorCallback(const Callback &cb);           // 设置出错处理函数
    void SetConnectionCleanUp(const Callback &cb);       // 设置连接清空函数，此函数独属于TcpServer
    void SetReqHandler(const Callback &cb);              // 设置本次连接事件请求的处理函数
    const Callback &GetReqHandler();                     // 获取本次连接事件请求的处理函数
    void SetBindedHandler(const bool BindedHandler);     // 设置处理函数绑定状态
    bool GetBindedHandler(const bool BindedHandler);     // 获取处理函数绑定状态
    int SetSendMessage(const std::string &newMsg);       // 重置bufferOut_的内容
    int AddSendMessage(const std::string &newMsg);       // 添加新数据到bufferOut_
    void SetDynamicHandler(const Callback &cb);          // 设置向TcpServer申请动态绑定函数的函数
    void SetAsyncProcessing(const bool asyncProcessing); // 设置异步处理标志

private:
    std::mutex mutex_;                        // 锁
    EventLoop *loop_;                         // 处理当前TcpConnection的事件池指针
    int fd_;                                  // 客户端连接套接字描述符
    bool ChannelAdded_;                       // 当前spChannel_是否已添加到TcpServer->Channel->Poller下进行监听
    struct sockaddr_in clientAddr_;           // 连接信息结构体
    bool disConnected_;                       // 连接断开标志位
    bool halfClose_;                          // 半关闭标志位
    bool asyncProcessing_;                    // 异步调用标志位，当工作任务交给线程池时，置为true，任务完成回调时置为false
    bool keepalive_;                          // 长连接标志，一般用于HttpServer服务
    bool reqHealthy_;                         // 请求解析结果，代表解析是否正常
    Timer *timer_;                            // 定时器
    std::string bufferIn_;                    // 接收数据缓冲区
    std::string bufferOut_;                   // 发送数据缓冲区
    bool BindedHandler_;                      // 处理函数绑定标志
    std::unique_ptr<Channel> spChannel_;      // 连接Channel实例
    HttpRequestContext httpRequestContext_;   // 请求解析结构
    HttpResponseContext httpResponseContext_; // 响应结构
    Callback messageCallback_;                // 请求响应函数，每次请求都会重置
    Callback sendcompleteCallback_;           // 发送完毕处理函数，每次请求都会重置
    Callback closeCallback_;                  // 连接关闭处理函数，每次请求都会重置
    Callback errorCallback_;                  // 错误处理函数，每次请求都会重置
    Callback reqHandler_;                     // 本次连接事件请求的处理函数，每次请求都会重置
    Callback BindDynamicHandler_;             // 向TcpServer申请动态绑定函数，此函数独属于TcpServer
    Callback connectioncleanup_;              // 连接清理函数，此函数独属于TcpServer
};

TcpConnection::TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr)
    : loop_(loop),
      mutex_(),
      spChannel_(new Channel()),
      ChannelAdded_(false),
      fd_(fd),
      timer_(NULL),
      clientAddr_(clientaddr),
      halfClose_(false),
      disConnected_(false),
      asyncProcessing_(false),
      bufferIn_(),
      bufferOut_(),
      keepalive_(true),
      reqHealthy_(false),
      BindedHandler_(false)
{
    // 基于Channel设置TcpConnection的服务函数，在Channel内触发调用TcpConnection的成员函数，类似于信号槽机制
    spChannel_->SetFd(fd_);
    spChannel_->SetEvents(EPOLLIN | EPOLLET);
    spChannel_->SetReadHandle(std::bind(&TcpConnection::HandleRead, this));
    spChannel_->SetWriteHandle(std::bind(&TcpConnection::HandleWrite, this));
    spChannel_->SetCloseHandle(std::bind(&TcpConnection::HandleClose, this));
    spChannel_->SetErrorHandle(std::bind(&TcpConnection::HandleError, this));
}

TcpConnection::~TcpConnection()
{
    // 多线程下，加入loop的任务队列？不用，因为已经在当前loop线程
    // 移除事件，析构成员变量
    std::cout << "输出测试：一个TcpConnection连接已被废弃，开始析构, 连接sockfd：" << fd_ << std::endl;
    loop_->RemoveChannelToPoller(spChannel_.get());
    close(fd_);
    delete timer_;
    std::cout << "输出测试：一个TcpConnection连接已被废弃，析构即将结束, 连接sockfd：" << fd_ << std::endl;
}

/*
 * 接收客户端发送的数据，调用绑定的messageCallback_函数
 *
 */
void TcpConnection::HandleRead()
{
    std::cout << "输出测试：TcpConnection::HandleRead sockfd：" << fd_ << std::endl;
    // 接收数据，写入缓冲区bufferIn_
    int result = recvn(fd_, bufferIn_);
    if (result > 0)
    {
        if (disConnected_)
        {
            std::cout << "输出测试：TcpConnection::HandleRead 连接已关闭，不再处理该连接的请求，连接sockfd：" << fd_ << std::endl;
            return;
        }
        if (reqHealthy_ = ParseHttpRequest())
        {
            spTcpConnection sptcpconn = shared_from_this();
            bool preBindedHandler_ = BindedHandler_;
            // 在此向TcpServer请求函数绑定，需要先重置BindedHandler_为false以免复用连接时错误
            BindedHandler_ = false;
            BindDynamicHandler_(sptcpconn);
            if (!BindedHandler_)
            {
                std::cout << "输出测试：TcpConnection::HandleRead 动态绑定函数失败，处理错误，sockfd：" << fd_ << std::endl;
                if (preBindedHandler_)
                {
                    spTcpConnection sptcpconn = shared_from_this();
                    errorCallback_(sptcpconn);
                    closeCallback_(sptcpconn);
                }
                HandleError();
            }
            else
            {
                std::cout << "输出测试：TcpConnection::HandleRead 动态绑定函数完毕，调整定时关闭并回调高级服务处理，sockfd：" << fd_ << std::endl;
                if (!timer_)
                {
                    timer_ = new Timer(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, shared_from_this()));
                    timer_->Start();
                }
                else
                {
                    timer_->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, shared_from_this()));
                }
                std::cout << "输出测试：TcpConnection::HandleRead 回调高级服务处理，sockfd：" << fd_ << std::endl;
                // 执行动态绑定的上层处理函数messageCallback_处理读取到的缓冲区数据bufferIn_
                messageCallback_(sptcpconn);
            }
        }
        else
        {
            std::cout << "输出测试：TcpConnection::HandleRead 解析请求失败，处理错误，sockfd：" << fd_ << std::endl;
            HandleError();
        }
    }
    else if (result == 0)
    {
        std::cout << "输出测试：TcpConnection::HandleRead 未读取到任何数据，关闭连接，sockfd：" << fd_ << std::endl;
        HandleClose();
    }
    else
    {
        std::cout << "输出测试：TcpConnection::HandleRead 读取数据失败，错误处理，sockfd：" << fd_ << std::endl;
        HandleError();
    }
}

/*
 * EventLoop添加监听Channel
 * 实际由EventLoop下Poller添加新监听连接
 */
void TcpConnection::AddChannelToLoop()
{
    std::cout << "输出测试：TcpConnection::AddChannelToLoop sockfd：" << fd_ << std::endl;
    loop_->AddTask(std::bind(&EventLoop::AddChannelToPoller, loop_, spChannel_.get()));
    ChannelAdded_ = true;
}

/*
 * 发送信息函数，指定EventLoop执行
 *
 */
void TcpConnection::Send(const std::string &s)
{
    std::cout << "输出测试：TcpConnection::Send sockfd：" << fd_ << std::endl;
    SetSendMessage(s);
    SendBufferOut();
}

/*
 * 发送信息函数，指定EventLoop执行
 * 传递的数据为char*类型，若不指定数据长度，会被第一个'\0'截断
 *
 */
void TcpConnection::Send(const char *s, int length)
{
    std::cout << "输出测试：TcpConnection::Send sockfd：" << fd_ << std::endl;
    if (!length)
    {
        // 缺省默认长度为0，只能用strlen函数计算s的长度，这会被第一个'\0'截断
        length = strlen(s);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    bufferOut_.clear();
    bufferOut_.append(s, length);
    SendBufferOut();
}

/*
 * 发送信息函数，指定EventLoop执行
 *
 */
void TcpConnection::SendBufferOut()
{
    std::cout << "输出测试：TcpConnection::SendBufferOut sockfd：" << fd_ << std::endl;
    // 判断当前线程是不是Loop IO所在线程
    if (loop_->GetThreadId() == std::this_thread::get_id())
    {
        SendInLoop();
    }
    else
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 当前线程为新开线程
        asyncProcessing_ = false;
        // 跨线程调用,加入IO线程的任务队列，唤醒
        spTcpConnection sptcpconn = shared_from_this();
        std::cout << "输出测试：TcpConnection::SendBufferOut 向loop_添加TcpConnection::SendInLoop函数，socket：" << fd_ << std::endl;
        loop_->AddTask(std::bind(&TcpConnection::SendInLoop, sptcpconn));
    }
}

/*
 * 发送信息函数，由EventLoop执行
 *
 */
void TcpConnection::SendInLoop()
{
    std::cout << "输出测试：TcpConnection::SendInLoop sockfd：" << fd_ << std::endl;
    if (disConnected_)
    {
        std::cout << "输出测试：TcpConnection::SendInLoop 连接已关闭，无法发送任何数据，连接sockfd：" << fd_ << std::endl;
        return;
    }
    int result = sendn(fd_, bufferOut_);
    if (result > 0)
    {
        uint32_t events = spChannel_->GetEvents();
        if (!bufferOut_.empty())
        {
            // 缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
            spChannel_->SetEvents(events | EPOLLOUT);
            loop_->UpdateChannelToPoller(spChannel_.get());
        }
        else
        {
            // 数据已发完
            spChannel_->SetEvents(events & (~EPOLLOUT));
            if (BindedHandler_)
            {
                spTcpConnection sptcpconn = shared_from_this();
                sendcompleteCallback_(sptcpconn);
            }
            // 已设置半关闭标志，连接即将关闭
            if (halfClose_)
                HandleClose();
        }
    }
    else if (result < 0)
    {
        std::cout << "输出测试：TcpConnection::SendInLoop 发送数据失败，错误处理并关闭连接" << std::endl;
        HandleError();
    }
    else
    {
        std::cout << "输出测试：TcpConnection::SendInLoop 发送数据量为0，关闭连接" << std::endl;
        HandleClose();
    }
}

/*
 * 关闭当前连接，指定EventLoop执行HandleClose函数
 *
 */
void TcpConnection::Shutdown()
{
    std::cout << "输出测试：TcpConnection::Shutdown sockfd：" << fd_ << std::endl;
    if (loop_->GetThreadId() == std::this_thread::get_id())
    {
        HandleClose();
    }
    else
    {
        // 加入IO线程的任务队列，唤醒
        spTcpConnection sptcpconn = shared_from_this();
        std::cout << "输出测试：TcpConnection::Shutdown 向loop_添加TcpConnection::HandleClose函数" << std::endl;
        loop_->AddTask(std::bind(&TcpConnection::HandleClose, sptcpconn));
    }
}

/*
 * 向客户端发送数据
 *
 */
void TcpConnection::HandleWrite()
{
    std::cout << "输出测试：TcpConnection::HandleWrite sockfd：" << fd_ << std::endl;
    if (disConnected_)
    {
        std::cout << "输出测试：TcpConnection::HandleWrite 连接已关闭，无法发送任何数据，连接sockfd：" << fd_ << std::endl;
        return;
    }
    int result = sendn(fd_, bufferOut_);
    if (result > 0)
    {
        uint32_t events = spChannel_->GetEvents();
        if (!bufferOut_.empty())
        {
            // 缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
            spChannel_->SetEvents(events | EPOLLOUT);
            loop_->UpdateChannelToPoller(spChannel_.get());
        }
        else
        {
            // 数据已发完
            spChannel_->SetEvents(events & (~EPOLLOUT));
            spTcpConnection sptcpconn = shared_from_this();
            sendcompleteCallback_(sptcpconn);
            // 发送完毕，如果是半关闭状态，则可以close了
            if (halfClose_)
                HandleClose();
        }
    }
    else if (result < 0)
    {
        std::cout << "输出测试：TcpConnection::HandleWrite 数据发送失败，错误处理" << std::endl;
        HandleError();
    }
    else
    {
        std::cout << "输出测试：TcpConnection::HandleWrite 数据发送量为0，关闭连接" << std::endl;
        HandleClose();
    }
}

/*
 * 处理连接错误
 *
 */
void TcpConnection::HandleError()
{
    std::cout << "输出测试：TcpConnection::HandleError sockfd：" << fd_ << std::endl;
    if (disConnected_)
    {
        std::cout << "输出测试：TcpConnection::HandleError 连接已关闭，无法发送任何数据，连接sockfd：" << fd_ << std::endl;
        return;
    }
    if (!BindedHandler_)
    {
        // 未绑定处理函数或本次请求的函数绑定失败，客户端函数写错了
        std::cout << "输出测试：绑定函数失败，本次请求url：" << httpRequestContext_.url << std::endl;
        if (!httpRequestContext_.serviceName.empty())
        {
            HttpError(400, "所请求的服务：" + httpRequestContext_.serviceName + " 没有这样的处理函数（" + httpRequestContext_.handlerName + "）：" + httpRequestContext_.url);
        }
        else
        {
            HttpError(400, "请求报文语法有误，服务器无法识别：" + httpRequestContext_.url);
        }
    }
    else if (disConnected_)
    {
        // 连接已被HandleClose处理关闭了
        return;
    }
    else
    {
        // 连接尚未关闭，调用可调用的errorCallback_，并关闭连接
        std::cout << "输出测试：TcpConnection错误处理，socket：" << fd_ << std::endl;
        spTcpConnection sptcpconn = shared_from_this();
        errorCallback_(sptcpconn);
        HandleClose();
    }
}

/*
 * 处理客户端连接关闭
 * 对端关闭连接,有两种，一种close，另一种是shutdown(半关闭)
 * 但服务器并不清楚是哪一种，只能按照最保险的方式来，即发完数据再close
 */
void TcpConnection::HandleClose()
{
    std::cout << "输出测试：TcpConnection::HandleClose sockfd：" << fd_ << std::endl;
    if (disConnected_)
    {
        std::cout << "输出测试：TcpConnection::HandleClose 无需处理已关闭的连接TcpConnection，socket：" << fd_ << std::endl;
        return;
    }
    std::cout << "输出测试：TcpConnection::HandleClose TcpConnection连接即将关闭，socket：" << fd_ << std::endl;
    // if (bufferOut_.size() > 0 || bufferIn_.length() > 0 || asyncProcessing_)
    if (asyncProcessing_)
    {
        std::cout << "输出测试：TcpConnection::HandleClose TcpConnection连接未正常处理，半关闭并处理，socket：" << fd_ << std::endl;
        // 有线程正在逻辑处理
        halfClose_ = true;
        if (bufferIn_.length() > 0)
        {
            spTcpConnection sptcpconn = shared_from_this();
            messageCallback_(sptcpconn);
        }
    }
    else
    {
        spTcpConnection sptcpconn = shared_from_this();
        if (BindedHandler_)
            closeCallback_(sptcpconn);
        std::cout << "输出测试：TcpConnection::HandleClose 向loop_添加TcpConnection::connectioncleanup_函数，sockfd：" << fd_ << std::endl;
        // 在TcpServer内删除指向此TcpConnection的智能指针，在所有多线程任务都执行完后，没有指向此的智能指针时将进行析构
        loop_->AddTask(std::bind(connectioncleanup_, sptcpconn));
        disConnected_ = true;
    }
}

/*
 * 设置向TcpServer申请动态绑定函数的函数
 *
 */
void TcpConnection::SetDynamicHandler(const Callback &cb)
{
    std::cout << "输出测试：TcpConnection::SetDynamicHandler sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    BindDynamicHandler_ = cb;
}

/*
 * 设置连接处理函数
 *
 */
void TcpConnection::SetMessaeCallback(const Callback &cb)
{
    std::cout << "输出测试：TcpConnection::SetMessaeCallback sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    messageCallback_ = cb;
}

/*
 * 设置数据发送完毕处理函数
 *
 */
void TcpConnection::SetSendCompleteCallback(const Callback &cb)
{
    std::cout << "输出测试：TcpConnection::SetSendCompleteCallback sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    sendcompleteCallback_ = cb;
}

/*
 * 设置关闭处理函数
 *
 */
void TcpConnection::SetCloseCallback(const Callback &cb)
{
    std::cout << "输出测试：TcpConnection::SetCloseCallback sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    closeCallback_ = cb;
}

/*
 * 设置出错处理函数
 *
 */
void TcpConnection::SetErrorCallback(const Callback &cb)
{
    std::cout << "输出测试：TcpConnection::SetErrorCallback sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    errorCallback_ = cb;
}

/*
 * 设置连接清空函数，此函数独属于TcpServer
 *
 */
void TcpConnection::SetConnectionCleanUp(const Callback &cb)
{
    std::cout << "输出测试：TcpConnection::SetConnectionCleanUp sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    connectioncleanup_ = cb;
}

/*
 * 设置本次连接事件请求的处理函数
 *
 */
void TcpConnection::SetReqHandler(const Callback &cb)
{
    std::cout << "输出测试：TcpConnection::SetReqHandler sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    reqHandler_ = cb;
}

/*
 * 获取本次连接事件请求的处理函数
 *
 */
const TcpConnection::Callback &TcpConnection::GetReqHandler()
{
    std::cout << "输出测试：TcpConnection::GetReqHandler sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    return reqHandler_;
}

/*
 * 设置处理函数绑定状态
 *
 */
void TcpConnection::SetBindedHandler(const bool BindedHandler)
{
    std::cout << "输出测试：TcpConnection::SetBindedHandler sockfd：" << fd_ << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    BindedHandler_ = BindedHandler;
}

/*
 * 获取处理函数绑定状态
 *
 */
bool TcpConnection::GetBindedHandler(const bool BindedHandler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return BindedHandler_;
}

/*
 * 获取连接处理函数
 *
 */
TcpConnection::Callback TcpConnection::GetMessageCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return messageCallback_;
}

/*
 * 获取数据发送完毕处理函数
 *
 */
TcpConnection::Callback TcpConnection::GetSendCompleteCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return sendcompleteCallback_;
}

/*
 * 获取关闭处理函数
 *
 */
TcpConnection::Callback TcpConnection::GetCloseCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return closeCallback_;
}

/*
 * 获取出错处理函数
 *
 */
TcpConnection::Callback TcpConnection::GetErrorCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return errorCallback_;
}

/*
 * 获取连接清空函数
 *
 */
TcpConnection::Callback TcpConnection::GetConnectionCleanUp()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connectioncleanup_;
}

/*
 * 设置异步处理标志
 *
 */
void TcpConnection::SetAsyncProcessing(const bool asyncProcessing)
{
    std::lock_guard<std::mutex> lock(mutex_);
    asyncProcessing_ = asyncProcessing;
}

/*
 * 重置bufferOut_的内容
 * 传递的数据为char*类型，若不指定数据长度，会被第一个'\0'截断
 *
 */
int TcpConnection::SetSendMessage(const std::string &newMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bufferOut_.clear();
    bufferOut_.append(newMsg, 0, newMsg.length());
    return newMsg.length();
}

/*
 * 添加新数据到bufferOut_
 * 传递的数据为char*类型，若不指定数据长度，会被第一个'\0'截断
 *
 */
int TcpConnection::AddSendMessage(const std::string &newMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bufferOut_.append(newMsg, 0, newMsg.length());
    return newMsg.length();
}

/*
 * 获取接收缓冲区的指针
 *
 */
std::string &TcpConnection::GetBufferIn()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return bufferIn_;
}

/*
 * 获取接收缓冲区的指针
 *
 */
std::string &TcpConnection::GetBufferOut()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return bufferOut_;
}

/*
 * 获取接收到的数据的长度
 *
 */
int TcpConnection::GetReceiveLength()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return bufferIn_.size();
}

/*
 * 获取接收到的数据的长度
 *
 */
int TcpConnection::GetSendLength()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return bufferOut_.length();
}

/*
 * 判断连接是否已关闭，是则返回true，否则返回false
 *
 */
bool TcpConnection::IsDisconnected()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return disConnected_;
}

/*
 * 获取长连接标志
 *
 */
bool TcpConnection::WillKeepAlive()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return keepalive_;
}

/*
 * 获取连接请求解析结果状态
 *
 */
bool TcpConnection::GetReqHealthy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return reqHealthy_;
}

/*
 * 设置长连接标志
 *
 */
void TcpConnection::SetKeepAlive(bool keepalive)
{
    std::lock_guard<std::mutex> lock(mutex_);
    keepalive_ = keepalive;
}

/*
 * 启动定时器
 *
 */
void TcpConnection::StartTimer()
{
    timer_->Start();
}

/*
 * 获取定时器指针
 *
 */
Timer *TcpConnection::GetTimer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return timer_;
}

/*
 * 获取请求解析结构体的引用
 *
 */
HttpRequestContext &TcpConnection::GetReqestBuffer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return httpRequestContext_;
}

/*
 * 获取响应解析结构体的引用
 *
 */
HttpResponseContext &TcpConnection::GetResonseBuffer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return httpResponseContext_;
}

/*
 * 解析http请求信息
 *
 */
bool TcpConnection::ParseHttpRequest()
{
    std::cout << "输出测试：TcpConnection::ParseHttpRequest sockfd：" << fd_ << std::endl;
    std::string crlf("\r\n"), crlfcrlf("\r\n\r\n");
    size_t prev = 0, next = 0, pos_colon;
    std::string key, value;
    bool parseresult = false;
    // TODO以下解析可以改成状态机，解决一次收Http报文不完整问题
    if ((next = bufferIn_.find(crlf, prev)) != std::string::npos)
    {
        // 有至少一个"\r\n"字段
        std::string first_line(bufferIn_.substr(prev, next - prev));
        prev = next;
        std::stringstream sstream(first_line);
        sstream >> (httpRequestContext_.method);
        sstream >> (httpRequestContext_.url);
        sstream >> (httpRequestContext_.version);
    }
    else
    {
        // 没有"\r\n"字段
        std::cout << "error received message：" << bufferIn_ << std::endl;
        std::cout << "Error in httpParser: http_request_line isn't complete!" << std::endl;
        parseresult = false;
        bufferIn_.clear();
        return parseresult;
        // TODO 可以临时存起来，凑齐了再解析
    }
    size_t pos_crlfcrlf = 0;
    if ((pos_crlfcrlf = bufferIn_.find(crlfcrlf, prev)) != std::string::npos)
    {
        // 有"\r\n\r\n"字段
        while (prev != pos_crlfcrlf)
        {
            next = bufferIn_.find(crlf, prev + 2);
            pos_colon = bufferIn_.find(":", prev + 2);
            key = bufferIn_.substr(prev + 2, pos_colon - prev - 2);
            value = bufferIn_.substr(pos_colon + 2, next - pos_colon - 2);
            prev = next;
            httpRequestContext_.header.insert(std::pair<std::string, std::string>(key, value));
        }
    }
    else
    {
        // 无"\r\n\r\n"字段
        std::cout << "Error in httpParser: http_request_header isn't complete!" << std::endl;
        parseresult = false;
        bufferIn_.clear();
        return parseresult;
    }
    httpRequestContext_.body = bufferIn_.substr(pos_crlfcrlf + 4);
    parseresult = true;
    bufferIn_.clear();
    return parseresult;
}

/*
 * 解析http请求信息
 * 该版函数已弃用
 *
 */
// bool TcpConnection::ParseHttpRequest(char *msg, int msgLength, HttpRequestContext &httprequestcontext)
// {
//     std::cout << "输出测试：TcpConnection::ParseHttpRequest sockfd：" << fd_ << std::endl;;
//     const char *crlf = "\r\n";
//     const char *crlfcrlf = "\r\n\r\n";
//     bool parseresult = false;
//     char *preFind = msg, *nextFind = NULL, *pos_colon = nullptr;
//     std::string key, value;
//     char *const pos_crlfcrlf = strstr(preFind, crlfcrlf);
//     char buffer[BUFSIZE];
//     // TODO 以下解析可以改成状态机，解决一次收Http报文不完整问题
//     if(nextFind = strstr(preFind, crlf))
//     {
//         memcpy(buffer, preFind, nextFind - preFind);
//         std::string first_line(buffer, nextFind - preFind);
//         preFind = nextFind + 2;
//         std::stringstream sstream(first_line);
//         sstream >> (httprequestcontext.method);
//         sstream >> (httprequestcontext.url);
//         sstream >> (httprequestcontext.version);
//     }
//     else
//     {
//         std::cout << "接收到信息：" << msg << std::endl;
//         std::cout << "Error in httpParser: http_request_line 不完整!" << std::endl;
//         parseresult = false;
//         return parseresult;
//         //可以临时存起来，凑齐了再解析
//     }
//     if(pos_crlfcrlf)
//     {
//         while(pos_crlfcrlf != (nextFind = strstr(preFind, crlf)))
//         {
//             // 仍在查询请求头部分
//             if(nextFind)
//             {
//                 pos_colon = strstr(preFind + 2, ":");
//                 memcpy(buffer, preFind, pos_colon - preFind);
//                 key.clear();
//                 key.append(buffer, pos_colon - preFind);
//                 memcpy(buffer, pos_colon + 2, nextFind - (pos_colon + 2));
//                 value.clear();
//                 value.append(buffer, nextFind - (pos_colon + 2));
//                 preFind = nextFind + 2;
//                 httprequestcontext.header.insert(std::pair<std::string, std::string>(key, value));
//             }
//             else
//                 break;
//         }
//     }
//     else
//     {
//         std::cout << "接收到信息：" << msg << std::endl;
//         std::cout << "Error in httpParser: http_request_header 不完整!" << std::endl;
//         parseresult = false;
//         return parseresult;
//     }
//     std::string conLen = httprequestcontext.header["Content-Length"];
//     int contentLength = conLen.empty() ? msgLength - (pos_crlfcrlf + 4 - msg) : atoi(conLen.c_str());
//     memcpy(buffer, pos_crlfcrlf + 4, contentLength);
//     httprequestcontext.body.clear();
//     httprequestcontext.body.append(buffer, 0, contentLength);
//     parseresult = true;
//     return parseresult;
// }

/*
 * 处理错误http请求，返回错误描述
 *
 */
void TcpConnection::HttpError(const int err_num, const std::string &short_msg)
{
    std::cout << "输出测试：TcpConnection::HttpError sockfd：" << fd_ << std::endl;
    bufferOut_.clear();
    if (httpRequestContext_.version.empty())
    {
        bufferOut_ += "HTTP/1.1 " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    else
    {
        bufferOut_ += httpRequestContext_.version + " " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    bufferOut_ += "Server: Qiu Hai's NetServer/0.1\r\n";
    bufferOut_ += "Content-Type: text/html\r\n";
    bufferOut_ += "Connection: Keep-Alive\r\n";
    std::string responsebody;
    responsebody += "<html><title>出错了</title>";
    responsebody += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>";
    responsebody += "<style>body{background-color:#f;font-size:14px;}h1{font-size:60px;color:#eeetext-align:center;padding-top:30px;font-weight:normal;}</style>";
    responsebody += "<body bgcolor=\"ffffff\"><h1>";
    responsebody += std::to_string(err_num) + " " + short_msg;
    responsebody += "</h1><hr><em> Qiu Hai's NetServer</em>\n</body></html>";
    bufferOut_ += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
    bufferOut_ += "\r\n";
    bufferOut_.append(responsebody, 0, responsebody.length());
    SendBufferOut();
}

/*
 * 读取客户端数据
 *
 */
int TcpConnection::recvn(int fd, std::string &recvMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    recvMsg.clear();
    int nbyte = 0;
    char buffer[BUFSIZE];
    for (;;)
    {
        nbyte = read(fd, buffer, BUFSIZE);
        if (nbyte > 0)
        {
            recvMsg.append(buffer, nbyte);
            if (nbyte < BUFSIZE)
            {
                // std::cout << std::endl << "接收到请求内容：" << std::endl << recvMsg << std::endl << std::endl;
                return recvMsg.length(); // 读优化，减小一次读调用，因为一次调用耗时10+us
            }
            else
                continue;
        }
        else if (nbyte < 0) // 异常
        {
            if (errno == EAGAIN) // 系统缓冲区未有数据，非阻塞返回
            {
                // std::cout << "EAGAIN,系统缓冲区未有数据，非阻塞返回" << std::endl;
                return recvMsg.size();
            }
            else if (errno == EINTR)
            {
                std::cout << "errno == EINTR" << std::endl;
                continue;
            }
            else
            {
                // 可能是RST
                perror("recv error");
                // std::cout << "recv error" << std::endl;
                return -1;
            }
        }
        else // 返回0，客户端关闭socket，FIN
        {
            // std::cout << "client close the Socket" << std::endl;
            return 0;
        }
    }
}

/*
 * 发送数据到客户端
 *
 */
int TcpConnection::sendn(int fd, std::string &sendMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ssize_t nbyte = 0;
    int sendsum = 0;
    size_t length = sendMsg.length() > BUFSIZE ? BUFSIZE : sendMsg.size();
    for (;;)
    {
        nbyte = send(fd, sendMsg.data(), length, 0);
        sleep(0.1); // 防止毡包 TODO 设计更好的防护
        if (nbyte > 0)
        {
            sendsum += nbyte;
            sendMsg.erase(0, nbyte);
            length = sendMsg.length() > BUFSIZE ? BUFSIZE : sendMsg.size();
            if (!length)
                break;
        }
        else if (nbyte < 0) // 异常
        {
            if (errno == EAGAIN) // 系统缓冲区满，非阻塞返回
            {
                std::cout << "write errno == EAGAIN,not finish!" << std::endl;
                return sendsum;
            }
            else if (errno == EINTR)
            {
                std::cout << "write errno == EINTR" << std::endl;
                continue;
            }
            else if (errno == EPIPE)
            {
                // 客户端已经close，并发了RST，继续wirte会报EPIPE，返回0，表示close
                perror("write error");
                std::cout << "write errno == client send RST" << std::endl;
                return -1;
            }
            else
            {
                perror("write error"); // Connection reset by peer
                std::cout << "write error, unknow error" << std::endl;
                return -1;
            }
        }
        else // 返回0
        {
            // 应该不会返回0
            // std::cout << "client close the Socket!" << std::endl;
            return 0;
        }
    }
    return sendsum;
}
