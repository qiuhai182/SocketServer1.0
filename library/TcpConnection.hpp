
// TcpConnection类：
//  客户端连接的抽象表示

#pragma once

#include <iostream>
#include <functional>
#include <string>
#include <thread>
#include <memory>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Channel.hpp"
#include "EventLoop.hpp"

#define BUFSIZE 4096

int recvn(int fd, std::string &bufferin);
int sendn(int fd, std::string &bufferout);
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{ // 允许安全使用shared_ptr
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection; // 指向TcpConnection的智能指针
    typedef std::function<void(const spTcpConnection &)> Callback; // 回调函数
    typedef std::function<void(const spTcpConnection &, std::string &)> MessageCallback;    // 信息处理函数
    TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr);
    ~TcpConnection();
    int fd() const { return fd_; }  // 获取套接字描述符
    EventLoop *GetLoop() const { return loop_; } // 获取事件池指针
    void Send(const std::string &s);    // 发送信息函数，指定EventLoop执行
    void SendInLoop();          // 发送信息函数，由EventLoop执行
    void AddChannelToLoop();    // EventLoop添加监听Channel
    void Shutdown();            // 关闭当前连接，指定EventLoop执行
    void ShutdownInLoop();      // 关闭当前连接，由EventLoop执行
    void HandleRead();          // 接收客户端发送的数据，调用绑定的messageCallback_函数
    void HandleWrite();         // 向客户端发送数据
    void HandleError();         // 处理连接错误
    void HandleClose();         // 处理客户端连接关闭
    void SetMessaeCallback(const MessageCallback &cb);  // 设置连接处理函数
    void SetSendCompleteCallback(const Callback &cb);   // 设置数据发送完毕处理函数
    void SetCloseCallback(const Callback &cb);          // 设置关闭处理函数
    void SetErrorCallback(const Callback &cb);          // 设置出错处理函数
    void SetConnectionCleanUp(const Callback &cb);      // 设置连接清空函数
    void SetAsyncProcessing(const bool asyncProcessing);// 设置异步处理标志

private:
    EventLoop *loop_;   // 事件池
    std::unique_ptr<Channel> spChannel_;    // 连接Channel实例
    int fd_;    // 连接套接字描述符
    struct sockaddr_in clientAddr_; // 连接信息结构体
    bool disConnected_;     // 连接断开标志位
    bool halfClose_;        // 半关闭标志位
    bool asyncProcessing_;  // 异步调用标志位，当工作任务交给线程池时，置为true，任务完成回调时置为false
    std::string bufferIn_;  // 
    std::string bufferOut_; // 
    MessageCallback messageCallback_;   // 
    Callback sendcompleteCallback_;     // 
    Callback closeCallback_;            // 
    Callback errorCallback_;            // 
    Callback connectioncleanup_;        // 

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
      bufferOut_()
{
    // 注册事件执行函数
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
}

/*
 * EventLoop添加监听Channel
 * 实际由EventLoop下Poller添加新监听连接
 */
void TcpConnection::AddChannelToLoop()
{
    // bug segement fault
    // https:// blog.csdn.net/littlefang/article/details/37922113
    // 多线程下，加入loop的任务队列
    // 主线程直接执行
    // loop_->AddChannelToPoller(pchannel_);
    loop_->AddTask(std::bind(&EventLoop::AddChannelToPoller, loop_, spChannel_.get()));
}

/*
 * 发送信息函数，指定EventLoop执行
 * 
 */
void TcpConnection::Send(const std::string &s)
{
    bufferOut_ += s; // 跨线程消息投递成功
    // 判断当前线程是不是Loop IO线程
    if (loop_->GetThreadId() == std::this_thread::get_id())
    {
        SendInLoop();
    }
    else
    {
        // 当前线程为新开线程，异步调用结束
        asyncProcessing_ = false;
        // 跨线程调用,加入IO线程的任务队列，唤醒
        loop_->AddTask(std::bind(&TcpConnection::SendInLoop, shared_from_this())); 
    }
}

/*
 * 发送信息函数，由EventLoop执行
 * 
 */
void TcpConnection::SendInLoop()
{
    // bufferOut_ += s;// copy一次
    if (disConnected_)
    {
        return;
    }
    int result = sendn(fd_, bufferOut_);
    if (result > 0)
    {
        uint32_t events = spChannel_->GetEvents();
        if (bufferOut_.size() > 0)
        {
            // 缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
            spChannel_->SetEvents(events | EPOLLOUT);
            loop_->UpdateChannelToPoller(spChannel_.get());
        }
        else
        {
            // 数据已发完
            spChannel_->SetEvents(events & (~EPOLLOUT));
            sendcompleteCallback_(shared_from_this());
            if (halfClose_)
                HandleClose();
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
        // 不是IO线程，则是跨线程调用，加入IO线程的任务队列，唤醒
        loop_->AddTask(std::bind(&TcpConnection::ShutdownInLoop, shared_from_this()));
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
    closeCallback_(shared_from_this());
    loop_->AddTask(std::bind(connectioncleanup_, shared_from_this()));
    disConnected_ = true;
}

/*
 * 接收客户端发送的数据，调用绑定的messageCallback_函数
 * 
 */
void TcpConnection::HandleRead()
{
    // 接收数据，写入缓冲区
    int result = recvn(fd_, bufferIn_);
    // 业务回调,可以利用工作线程池处理，投递任务
    if (result > 0)
    {
        messageCallback_(shared_from_this(), bufferIn_); // 可以用右值引用优化，bufferIn_.clear();
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
        if (bufferOut_.size() > 0)
        {
            // 缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
            spChannel_->SetEvents(events | EPOLLOUT);
            loop_->UpdateChannelToPoller(spChannel_.get());
        }
        else
        {
            // 数据已发完
            spChannel_->SetEvents(events & (~EPOLLOUT));
            sendcompleteCallback_(shared_from_this());
            // 发送完毕，如果是半关闭状态，则可以close了
            if (halfClose_)
                HandleClose();
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
    if (disConnected_)
    {
        return;
    }
    errorCallback_(shared_from_this());
    // loop_->RemoveChannelToPoller(pchannel_);
    // 连接标记为清理
    // task添加
    loop_->AddTask(std::bind(connectioncleanup_, shared_from_this())); // 自己不能清理自己，交给loop执行，Tcpserver清理
    disConnected_ = true;
}

/*
 * 处理客户端连接关闭
 * 对端关闭连接,有两种，一种close，另一种是shutdown(半关闭)
 * 但服务器并不清楚是哪一种，只能按照最保险的方式来，即发完数据再close 
 */
void TcpConnection::HandleClose()
{
    // 移除事件
    // loop_->RemoveChannelToPoller(pchannel_);
    // 连接标记为清理
    // task添加
    // loop_->AddTask(connectioncleanup_);
    // closeCallback_(this);
    if (disConnected_)
    {
        return;
    }
    if (bufferOut_.size() > 0 || bufferIn_.size() > 0 || asyncProcessing_)
    {
        // 如果还有数据待发送，则先发完,设置半关闭标志位
        halfClose_ = true;
        // 还有数据刚刚才收到，但同时又收到FIN
        if (bufferIn_.size() > 0)
        {
            messageCallback_(shared_from_this(), bufferIn_);
        }
    }
    else
    {
        loop_->AddTask(std::bind(connectioncleanup_, shared_from_this()));
        closeCallback_(shared_from_this());
        disConnected_ = true;
    }
}

/*
 * 设置连接处理函数
 * 
 */
void TcpConnection::SetMessaeCallback(const MessageCallback &cb)
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
 * 设置连接清空函数
 * 
 */
void TcpConnection::SetConnectionCleanUp(const Callback &cb)
{
    connectioncleanup_ = cb;
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
 * 读取客户端数据
 * 
 */
int recvn(int fd, std::string &bufferin)
{
    int nbyte = 0;
    int readsum = 0;
    char buffer[BUFSIZE];
    for (;;)
    {
        // nbyte = recv(fd, buffer, BUFSIZE, 0);
        nbyte = read(fd, buffer, BUFSIZE);
        if (nbyte > 0)
        {
            bufferin.append(buffer, nbyte); // 效率较低，2次拷贝
            readsum += nbyte;
            if (nbyte < BUFSIZE)
                return readsum; // 读优化，减小一次读调用，因为一次调用耗时10+us
            else
                continue;
        }
        else if (nbyte < 0) // 异常
        {
            if (errno == EAGAIN) // 系统缓冲区未有数据，非阻塞返回
            {
                // std::cout << "EAGAIN,系统缓冲区未有数据，非阻塞返回" << std::endl;
                return readsum;
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
int sendn(int fd, std::string &bufferout)
{
    ssize_t nbyte = 0;
    int sendsum = 0;
    // char buffer[BUFSIZE+1];
    size_t length = 0;
    // length = bufferout.copy(buffer, BUFSIZE, 0);
    // buffer[length] = '\0';
    //  if(bufferout.size() >= BUFSIZE)
    //  {
    //  	length =  BUFSIZE;
    //  }
    //  else
    //  {
    //  	length =  bufferout.size();
    //  }
    // 无拷贝优化
    length = bufferout.size();
    if (length >= BUFSIZE)
    {
        length = BUFSIZE;
    }
    for (;;)
    {
        // nbyte = send(fd, buffer, length, 0);
        // nbyte = send(fd, bufferout.c_str(), length, 0);
        nbyte = write(fd, bufferout.c_str(), length);
        if (nbyte > 0)
        {
            sendsum += nbyte;
            bufferout.erase(0, nbyte);
            // length = bufferout.copy(buffer, BUFSIZE, 0);
            // buffer[length] = '\0';
            length = bufferout.size();
            if (length >= BUFSIZE)
            {
                length = BUFSIZE;
            }
            if (length == 0)
            {
                return sendsum;
            }
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
}
