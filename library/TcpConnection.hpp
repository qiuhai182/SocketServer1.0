
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

int recvn(int fd, char *recvMsg, int &msgLength);
int sendn(int fd, char *sendMsg, int &msgLength);

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{ // 允许安全使用shared_ptr
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection; // 指向TcpConnection的智能指针
    typedef std::function<void(const spTcpConnection &)> Callback; // 回调函数
    typedef std::function<void(const spTcpConnection &, char *)> MessageCallback;    // 信息处理函数
    TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr);
    ~TcpConnection();
    int fd() const { return fd_; }  // 获取套接字描述符
    EventLoop *GetLoop() const { return loop_; }// 获取事件池指针
    void Send(const std::string &s);            // 发送信息函数，指定EventLoop执行
    void Send(const char *s, int length = 0);   // 发送信息函数，指定EventLoop执行
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
    int getReceiveLength();     // 获取接收到的数据的长度
    int setSendMessage(const char *newMsg, const int msgLen = 0);   // 重置bufferOut_的内容
    int addSendMessage(const char *newMsg, const int msgLen = 0);   // 添加新数据到bufferOut_

private:
    EventLoop *loop_;   // 事件池
    std::unique_ptr<Channel> spChannel_;    // 连接Channel实例
    int fd_;    // 客户端连接套接字描述符
    struct sockaddr_in clientAddr_; // 连接信息结构体
    bool disConnected_;     // 连接断开标志位
    bool halfClose_;        // 半关闭标志位
    bool asyncProcessing_;  // 异步调用标志位，当工作任务交给线程池时，置为true，任务完成回调时置为false
    char *bufferIn_;        // 接收数据缓冲区
    char *bufferOut_;       // 发送数据缓冲区
    int bufferInLen_;       // 接收数据缓冲区有效数据长度
    int bufferOutLen_;      // 发送数据缓冲区有效数据长度
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
      bufferIn_(nullptr),
      bufferOut_(NULL)
{
    // 基于Channel设置TcpConnection服务函数，在Channel内触发调用TcpConnectionr的成员函数，类似于信号槽机制
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
    delete bufferIn_;
    delete bufferOut_;
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
 * 传递的数据为string类型，转为char*时会被第一个'\0'截断
 * 
 */
void TcpConnection::Send(const std::string &s)
{
    // TODO 线程安全？
    if(bufferOut_)
    {
        delete bufferOut_;
    }
    bufferOut_ = new char();
    memcpy(bufferOut_, s.data(), bufferOutLen_ = s.size());
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
        loop_->AddTask(std::bind(&TcpConnection::SendInLoop, shared_from_this())); 
    }
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
    memcpy(bufferOut_, s, bufferOutLen_ = length);
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
        loop_->AddTask(std::bind(&TcpConnection::SendInLoop, shared_from_this())); 
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
    int result = sendn(fd_, bufferOut_, bufferOutLen_);
    if (result > 0)
    {
        uint32_t events = spChannel_->GetEvents();
        if (bufferOutLen_ > 0)
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
    // 接收数据，写入缓冲区bufferIn_
    int result = recvn(fd_, bufferIn_, bufferInLen_);
    if (result > 0)
    {
        // 将读取到的缓冲区数据bufferIn_回调回动态绑定的上层处理函数messageCallback_
        messageCallback_(shared_from_this(), bufferIn_);
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
    int result = sendn(fd_, bufferOut_, bufferOutLen_);
    if (result > 0)
    {
        uint32_t events = spChannel_->GetEvents();
        if (bufferOutLen_ > 0)
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
    if (bufferOutLen_ > 0 || bufferInLen_ > 0 || asyncProcessing_)
    {
        // 如果还有数据待发送，则先发完,设置半关闭标志位
        halfClose_ = true;
        // 还有数据刚刚才收到，但同时又收到FIN
        if (bufferInLen_ > 0)
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
 * 重置bufferOut_的内容
 * 传递的数据为char*类型，若不指定数据长度，会被第一个'\0'截断
 * 
 */
int TcpConnection::setSendMessage(const char *newMsg, const int msgLen)
{
    if(!msgLen)
    {
        // 缺省默认长度为0，只能用strlen函数计算s的长度，这会被第一个'\0'截断
        bufferOutLen_ = strlen(newMsg);
    }
    else
    {
        bufferOutLen_ = msgLen;
    }
    memcpy(bufferOut_, newMsg, msgLen);
    return msgLen;
}

/*
 * 添加新数据到bufferOut_
 * 传递的数据为char*类型，若不指定数据长度，会被第一个'\0'截断
 * 
 */
int TcpConnection::addSendMessage(const char *newMsg, const int msgLen)
{
    if(!msgLen)
    {
        // 缺省默认长度为0，只能用strlen函数计算s的长度，这会被第一个'\0'截断
        bufferOutLen_ += strlen(newMsg);
    }
    else
    {
        bufferOutLen_ += msgLen;
    }
    memcpy(bufferOut_ + bufferOutLen_, newMsg, msgLen);
    return msgLen;
}

/*
 * 获取接收到的数据的长度
 * 
 */
int TcpConnection::getReceiveLength()
{
    return bufferInLen_;
}

/*
 * 读取客户端数据
 * 
 */
int recvn(int fd, char *recvMsg, int &msgLength)
{
    int nbyte = 0;
    int readsum = 0;
    msgLength = 0;
    for (;;)
    {
        // nbyte = recv(fd, recvMsg, BUFSIZE, 0);
        nbyte = read(fd, recvMsg, BUFSIZE);
        if (nbyte > 0)
        {
            readsum += nbyte;
            msgLength += nbyte;
            if (nbyte < BUFSIZE)
                return msgLength; // 读优化，减小一次读调用，因为一次调用耗时10+us
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
int sendn(int fd, const char *sendMsg, int &msgLength)
{
    ssize_t nbyte = 0;
    int sendsum = 0;
    size_t length = msgLength > BUFSIZE ? BUFSIZE : msgLength;
    for (;;)
    {
        nbyte = send(fd, sendMsg, length, 0);
        sleep(0.1); // 防止毡包 TODO 设计更好的防护
        if(nbyte > 0)
        {
            sendsum += nbyte;
            msgLength -= nbyte;
            length = msgLength > BUFSIZE ? BUFSIZE : msgLength;
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
