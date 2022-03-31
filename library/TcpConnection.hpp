
// TcpConnection类：
//  客户端连接的抽象表示，每一个客户端连接对应一个TcpConnection
//  每一个TcpConnection内部生成一个Channel实例用于tcp数据交互

#pragma once

#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <memory>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Timer.hpp"
#include "Channel.hpp"
#include "Resource.hpp"
#include "EventLoop.hpp"
#include "TypeIdentify.hpp"

#define BUFSIZE 4096

// http请求信息结构
typedef struct _HttpRequestContext
{
    std::string method;     // http方法（get、post等）
    std::string url;        // http url
    std::string serviceName;// url解析出请求的服务名
    std::string handlerName;// url解析出请求的服务的处理函数
    std::string resourceUrl;// url的"/服务名/函数名"后的部分
    std::string version;    // http version
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
    typedef std::shared_ptr<TcpConnection> spTcpConnection; // 指向TcpConnection的智能指针
    typedef std::function<void(spTcpConnection &)> Callback; // 回调函数
    TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr);
    ~TcpConnection();
    int fd() const { return fd_; }              // 获取套接字描述符
    EventLoop *GetLoop() const { return loop_; }// 获取事件池指针
    int recvn(int fd, std::string &recvMsg);    // 从客户端fd接收数据
    int sendn(int fd, std::string &sendMsg);    // 发送数据到客户端fd
    void Send(const std::string &s);            // 发送信息函数，指定EventLoop执行
    void Send(const char *s, int length = 0);   // 发送信息函数，指定EventLoop执行
    void SendBufferOut();                       // 发送信息函数，仅发送bufferOut_存储的内容，指定EventLoop执行
    bool ParseHttpRequest();                    // 解析http请求信息
    bool GetReqHealthy();                       // 获取连接请求解析结果状态
    void SendInLoop();                          // 发送信息函数，由EventLoop执行
    void AddChannelToLoop();                    // EventLoop添加监听Channel
    void Shutdown();                            // 关闭当前连接，指定EventLoop执行
    void ShutdownInLoop();                      // 关闭当前连接，由EventLoop执行
    void HandleRead();                          // 由TcpConnection的Channel调用，接收客户端发送的数据，再调用绑定的messageCallback_函数
    void HandleWrite();                         // 由TcpConnection的Channel调用，向客户端发送数据，再调用绑定的sendcompleteCallback_函数
    void HandleError();                         // 由TcpConnection的Channel调用，处理连接错误，再调用绑定的errorCallback_函数与connectioncleanup_函数
    void HandleClose();                         // 由TcpConnection的Channel调用，处理客户端连接关闭，再调用绑定的closeCallback_函数与connectioncleanup_函数
    std::string &GetBufferIn();                 // 获取接收缓冲区的指针
    std::string &GetBufferOut();                // 获取发送缓冲区的指针
    int GetReceiveLength();                     // 获取接收到的数据的长度
    int GetSendLength();                        // 获取待发送数据的长度
    Timer *GetTimer();                          // 获取定时器指针
    void StartTimer();                          // 启动定时器
    bool WillKeepAlive();                       // 获取长连接标志
    void SetKeepAlive(bool keepalive);          // 设置长连接标志
    HttpRequestContext &GetReq();               // 获取请求解析结构体的引用
    HttpResponseContext &GetRes();              // 获取响应解析结构体的引用
    Callback GetMessageCallback();              // 设置连接处理函数
    Callback GetSendCompleteCallback();         // 设置数据发送完毕处理函数
    Callback GetCloseCallback();                // 设置关闭处理函数
    Callback GetErrorCallback();                // 设置出错处理函数
    Callback GetConnectionCleanUp();            // 设置连接清理函数
    // 处理错误http请求，返回错误描述
    void HttpError(const int err_num, const std::string &short_msg);
    void SetMessaeCallback(const Callback &cb);         // 设置连接处理函数
    void SetSendCompleteCallback(const Callback &cb);   // 设置数据发送完毕处理函数
    void SetCloseCallback(const Callback &cb);          // 设置关闭处理函数
    void SetErrorCallback(const Callback &cb);          // 设置出错处理函数
    void SetConnectionCleanUp(const Callback &cb);      // 设置连接清空函数，此函数独属于TcpServer
    void SetReqHandler(const Callback &cb);             // 设置本次连接事件请求的处理函数
    const Callback &GetReqHandler();                    // 获取本次连接事件请求的处理函数
    void SetBindedHandler(const bool BindedHandler);    // 设置处理函数绑定状态
    bool GetBindedHandler(const bool BindedHandler);    // 获取处理函数绑定状态
    int SetSendMessage(const std::string &newMsg);      // 重置bufferOut_的内容
    int AddSendMessage(const std::string &newMsg);      // 添加新数据到bufferOut_
    void SetDynamicHandler(const Callback &cb);         // 设置向TcpServer申请动态绑定函数的函数
    void SetAsyncProcessing(const bool asyncProcessing);// 设置异步处理标志

private:
    EventLoop *loop_;   // 事件池
    std::unique_ptr<Channel> spChannel_;    // 连接Channel实例
    int fd_;            // 客户端连接套接字描述符
    struct sockaddr_in clientAddr_; // 连接信息结构体
    bool disConnected_;             // 连接断开标志位
    bool halfClose_;                // 半关闭标志位
    bool asyncProcessing_;          // 异步调用标志位，当工作任务交给线程池时，置为true，任务完成回调时置为false
    bool keepalive_;                // 长连接标志，一般用于HttpServer服务
    bool reqHealthy_;               // 请求解析结果，代表解析是否正常
    Timer *timer_;                  // 定时器
    std::string bufferIn_;          // 接收数据缓冲区
    std::string bufferOut_;         // 发送数据缓冲区
    HttpRequestContext httpRequestContext_;     // 请求解析结构
    HttpResponseContext httpResponseContext_;   // 响应结构
    bool BindedHandler_;            // 处理函数绑定标志
    Callback BindDynamicHandler_;   // 向TcpServer申请动态绑定函数
    Callback messageCallback_;      // 请求响应函数
    Callback sendcompleteCallback_; // 发送完毕处理函数
    Callback closeCallback_;        // 连接关闭处理函数
    Callback errorCallback_;        // 错误处理函数
    Callback connectioncleanup_;    // 连接清理函数，此函数独属于TcpServer
    Callback reqHandler_;           // 本次连接事件请求的处理函数

};

TcpConnection::TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr)
    : loop_(loop),
      spChannel_(new Channel()),
      fd_(fd),
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
    loop_->RemoveChannelToPoller(spChannel_.get());
    close(fd_);
    delete timer_;
}

/*
 * EventLoop添加监听Channel
 * 实际由EventLoop下Poller添加新监听连接
 */
void TcpConnection::AddChannelToLoop()
{
    loop_->AddTask(std::bind(&EventLoop::AddChannelToPoller, loop_, spChannel_.get()));
}

/*
 * 发送信息函数，指定EventLoop执行
 * 
 */
void TcpConnection::Send(const std::string &s)
{
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
    if(!length)
    {
        // 缺省默认长度为0，只能用strlen函数计算s的长度，这会被第一个'\0'截断
        length = strlen(s);
    }
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
    // 判断当前线程是不是Loop IO所在线程
    if (loop_->GetThreadId() == std::this_thread::get_id())
    {
        SendInLoop();
    }
    else
    {
        // 当前线程为新开线程
        asyncProcessing_ = false;
        // 跨线程调用,加入IO线程的任务队列，唤醒
        spTcpConnection sptcpconn = shared_from_this();
        loop_->AddTask(std::bind(&TcpConnection::SendInLoop, sptcpconn));
    }
}

/*
 * 发送信息函数，由EventLoop执行
 * 
 */
void TcpConnection::SendInLoop()
{
    if (disConnected_)
    {
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
            // 已设置半关闭标志，连接即将关闭
            if(halfClose_) HandleClose();
        }
    }
    else if (result < 0)
    {
        HandleError();
    }
    else
    {
        HandleClose();
    }
}

/*
 * 关闭当前连接，指定EventLoop执行
 * 
 */
void TcpConnection::Shutdown()
{
    if (loop_->GetThreadId() == std::this_thread::get_id())
    {
        ShutdownInLoop();
    }
    else
    {
        // 加入IO线程的任务队列，唤醒
        spTcpConnection sptcpconn = shared_from_this();
        loop_->AddTask(std::bind(&TcpConnection::ShutdownInLoop, sptcpconn));
    }
}

/*
 * 关闭当前连接，由EventLoop执行
 * 调用绑定的closeCallback_函数
 * 向EventLoop添加connectioncleanup_函数任务
 */
void TcpConnection::ShutdownInLoop()
{
    if (disConnected_)
    {
        return;
    }
    spTcpConnection sptcpconn = shared_from_this();
    if(BindedHandler_)
        closeCallback_(sptcpconn);
    loop_->AddTask(std::bind(connectioncleanup_, sptcpconn));
    disConnected_ = true;
}

/*
 * 接收客户端发送的数据，调用绑定的messageCallback_函数
 * 
 */
void TcpConnection::HandleRead()
{
    // 接收数据，写入缓冲区bufferIn_
    int result = recvn(fd_, bufferIn_);
    // 在此向TcpServer请求函数绑定
    spTcpConnection sptcpconn = shared_from_this();
    BindDynamicHandler_(sptcpconn);
    if(!BindedHandler_)
    {
        HandleError();
    }
    else if (result > 0)
    {
        reqHealthy_ = ParseHttpRequest();
        if(!timer_) 
            timer_ = new Timer(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, shared_from_this()));
        else
            timer_->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, shared_from_this()));
        // 将读取到的缓冲区数据bufferIn_回调回动态绑定的上层处理函数messageCallback_
        timer_->Start();
        std::cout << "输出测试：即将调用信息处理函数" << std::endl;
        messageCallback_(sptcpconn);
        std::cout << "输出测试：调用信息处理函数完毕" << std::endl;
    }
    else if (result == 0)
    {
        HandleClose();
    }
    else
    {
        HandleError();
    }
}

/*
 * 向客户端发送数据
 * 
 */
void TcpConnection::HandleWrite()
{
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
            if(halfClose_) HandleClose();
        }
    }
    else if (result < 0)
    {
        HandleError();
    }
    else
    {
        HandleClose();
    }
}

/*
 * 处理连接错误
 * 
 */
void TcpConnection::HandleError()
{
    if(!BindedHandler_)
    {
        std::cout << "输出测试：绑定函数失败，url=" << httpRequestContext_.url << std::endl;
        // 未绑定处理函数或本次请求的函数绑定失败，客户端函数写错了
        if(!httpRequestContext_.serviceName.empty())
        {
            HttpError(400, "所请求的服务：" + httpRequestContext_.serviceName + " 没有这样的处理函数：" + httpRequestContext_.url);
        }
        else
        {
            HttpError(400, "请求报文语法有误，服务器无法识别：" + httpRequestContext_.url);
        }
    }
    if (disConnected_)
    {
        return;
    }
    std::cout << "输出测试：TcpConnection错误处理，socket：" << fd_ << std::endl;
    spTcpConnection sptcpconn = shared_from_this();
    if(BindedHandler_)
        errorCallback_(sptcpconn);
    loop_->AddTask(std::bind(connectioncleanup_, sptcpconn)); // 自己不能清理自己，交给loop执行，Tcpserver清理
    disConnected_ = true;
}

/*
 * 处理客户端连接关闭
 * 对端关闭连接,有两种，一种close，另一种是shutdown(半关闭)
 * 但服务器并不清楚是哪一种，只能按照最保险的方式来，即发完数据再close 
 */
void TcpConnection::HandleClose()
{
    if (disConnected_)
    {
        return;
    }
    if (bufferOut_.size() > 0 || bufferIn_.length() > 0 || asyncProcessing_)
    {
        // 如果还有数据待发送、接收或处于异步处理状态，则设置半关闭标志位
        halfClose_ = true;
        // 还有数据刚刚才收到，但同时又收到FIN，继续接收数据
        if (bufferIn_.length() > 0)
        {
            spTcpConnection sptcpconn = shared_from_this();
            messageCallback_(sptcpconn);
        }
    }
    else
    {
        spTcpConnection sptcpconn = shared_from_this();
        if(BindedHandler_)
            closeCallback_(sptcpconn);
        disConnected_ = true;
        loop_->AddTask(std::bind(connectioncleanup_, sptcpconn));
    }
}

/*
 * 设置向TcpServer申请动态绑定函数的函数
 * 
 */
void TcpConnection::SetDynamicHandler(const Callback &cb)
{
    BindDynamicHandler_ = cb;
}

/*
 * 设置连接处理函数
 * 
 */
void TcpConnection::SetMessaeCallback(const Callback &cb)
{
    messageCallback_ = cb;
}

/*
 * 设置数据发送完毕处理函数
 * 
 */
void TcpConnection::SetSendCompleteCallback(const Callback &cb)
{
    sendcompleteCallback_ = cb;
}

/*
 * 设置关闭处理函数
 * 
 */
void TcpConnection::SetCloseCallback(const Callback &cb)
{
    closeCallback_ = cb;
}

/*
 * 设置出错处理函数
 * 
 */
void TcpConnection::SetErrorCallback(const Callback &cb)
{
    errorCallback_ = cb;
}

/*
 * 设置连接清空函数，此函数独属于TcpServer
 * 
 */
void TcpConnection::SetConnectionCleanUp(const Callback &cb)
{
    connectioncleanup_ = cb;
}

/*
 * 设置本次连接事件请求的处理函数
 * 
 */
void TcpConnection::SetReqHandler(const Callback &cb)
{
    reqHandler_ = cb;
}

/*
 * 获取本次连接事件请求的处理函数
 * 
 */
const TcpConnection::Callback &TcpConnection::GetReqHandler()
{
    return reqHandler_;
}

/*
 * 设置处理函数绑定状态
 * 
 */
void TcpConnection::SetBindedHandler(const bool BindedHandler)
{
    BindedHandler_ = BindedHandler;
}

/*
 * 获取处理函数绑定状态
 * 
 */
bool TcpConnection::GetBindedHandler(const bool BindedHandler)
{
    return BindedHandler_;
}

/*
 * 获取连接处理函数
 * 
 */
TcpConnection::Callback TcpConnection::GetMessageCallback()
{
    return messageCallback_;
}

/*
 * 获取数据发送完毕处理函数
 * 
 */
TcpConnection::Callback TcpConnection::GetSendCompleteCallback()
{
    return sendcompleteCallback_ ;
}

/*
 * 获取关闭处理函数
 * 
 */
TcpConnection::Callback TcpConnection::GetCloseCallback()
{
    return closeCallback_;
}

/*
 * 获取出错处理函数
 * 
 */
TcpConnection::Callback TcpConnection::GetErrorCallback()
{
    return errorCallback_;
}

/*
 * 获取连接清空函数
 * 
 */
TcpConnection::Callback TcpConnection::GetConnectionCleanUp()
{
    return connectioncleanup_;
}

/*
 * 设置异步处理标志
 * 
 */
void TcpConnection::SetAsyncProcessing(const bool asyncProcessing)
{
    asyncProcessing_ = asyncProcessing;
}

/*
 * 重置bufferOut_的内容
 * 传递的数据为char*类型，若不指定数据长度，会被第一个'\0'截断
 * 
 */
int TcpConnection::SetSendMessage(const std::string &newMsg)
{
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
    bufferOut_.append(newMsg, 0, newMsg.length());
    return newMsg.length();
}

/*
 * 获取接收缓冲区的指针
 * 
 */
std::string &TcpConnection::GetBufferIn()
{
    return bufferIn_;
}

/*
 * 获取接收缓冲区的指针
 * 
 */
std::string &TcpConnection::GetBufferOut()
{
    return bufferOut_;
}

/*
 * 获取接收到的数据的长度
 * 
 */
int TcpConnection::GetReceiveLength()
{
    return bufferIn_.size();
}

/*
 * 获取接收到的数据的长度
 * 
 */
int TcpConnection::GetSendLength()
{
    return bufferOut_.length();
}

/*
 * 获取长连接标志
 * 
 */
bool TcpConnection::WillKeepAlive()
{
    return keepalive_;
}

/*
 * 获取连接请求解析结果状态
 * 
 */
bool TcpConnection::GetReqHealthy()
{
    return reqHealthy_;
}

/*
 * 设置长连接标志
 * 
 */
void TcpConnection::SetKeepAlive(bool keepalive)
{
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
    return timer_;
}

/*
 * 获取请求解析结构体的引用
 * 
 */
HttpRequestContext &TcpConnection::GetReq()
{
    return httpRequestContext_;
}

/*
 * 获取响应解析结构体的引用
 * 
 */
HttpResponseContext &TcpConnection::GetRes()
{
    return httpResponseContext_;
}

/*
 * 解析http请求信息
 * 
 */
bool TcpConnection::ParseHttpRequest()
{
    std::string crlf("\r\n"), crlfcrlf("\r\n\r\n");
    size_t prev = 0, next = 0, pos_colon;
    std::string key, value;
    bool parseresult = false;
    //TODO以下解析可以改成状态机，解决一次收Http报文不完整问题
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
                std::cout << std::endl << "接收到请求内容：" << std::endl << recvMsg << std::endl << std::endl;
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
    ssize_t nbyte = 0;
    int sendsum = 0;
    size_t length = sendMsg.length() > BUFSIZE ? BUFSIZE : sendMsg.size();
    for (;;)
    {
        nbyte = send(fd, sendMsg.data(), length, 0);
        sleep(0.1); // 防止毡包 TODO 设计更好的防护
        if(nbyte > 0)
        {
            sendsum += nbyte;
            sendMsg.erase(0, nbyte);
            length = sendMsg.length() > BUFSIZE ? BUFSIZE : sendMsg.size();
            if(!length) break;
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
