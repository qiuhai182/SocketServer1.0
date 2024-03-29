
// Channel类（类似一个session）：
//  承载一个事件的套接字描述符fd和读写等事件events的连接控制类
//  与epoll_event直接交互
//  每一个连接对应生成一个Channel实例，并为之动态绑定套接字描述符、事件events等

//  EPOLLIN：   表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
//  EPOLLOUT：  表示对应的文件描述符可以写；
//  EPOLLPRI：  表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
//  EPOLLERR：  表示对应的文件描述符发生错误；
//  EPOLLRDHUP：表示对应的文件描述符读关闭；
//  EPOLLHUP：  表示对应的文件描述符被挂断；
//  EPOLLET：   将EPOLL设为边缘触发(Edge Triggered)模式，
//              这是相对于默认的水平触发(Level Triggered)模式来说的；
//  EPOLLONESHOT：  只监听一次事件，当监听完这次事件之后，
//                  如果还需要继续监听这个socket的话，
//                  需要再次把这个socket加入到EPOLL队列里；

#pragma once

#include <iostream>
#include <functional>
#include <sys/epoll.h>
#include "LogServer.hpp"

class Channel
{
public:
    typedef std::function<void()> Callback;
    Channel();
    ~Channel();
    void SetFd(int fd);                      // 设置连接套接字fd
    int GetFd() const;                       // 获取连接套接字fd
    void SetEvents(uint32_t events);         // 设置连接监听事件epoll_event
    uint32_t GetEvents() const;              // 获取连接事件epoll_event
    void SetReadHandle(const Callback &cb);  // 设置读事件（EPOLLIN）处理函数
    void SetWriteHandle(const Callback &cb); // 设置写事件（EPOLLOUT）处理函数
    void SetErrorHandle(const Callback &cb); // 设置出错处理函数
    void SetCloseHandle(const Callback &cb); // 设置连接关闭函数
    void HandleEvent();                      // 执行连接事件

private:
    int fd_;                // 连接套接字描述符
    uint32_t events_;       // 事件，一般情况下为epoll_event.events
    Callback readHandler_;  // 读取数据回调函数
    Callback writeHandler;  // 写数据回调函数
    Callback errorHandler_; // 错误处理回调函数
    Callback closeHandler_; // 关闭连接回调函数
    
};

Channel::Channel()
    : fd_(-1)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
}

Channel::~Channel()
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
}

/*
 * 设置连接套接字fd
 *
 */
void Channel::SetFd(int fd)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    fd_ = fd;
}

/*
 * 获取连接套接字fd
 *
 */
int Channel::GetFd() const
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    return fd_;
}

/*
 * 设置连接监听事件epoll_event
 *
 */
void Channel::SetEvents(uint32_t events)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    events_ = events;
}

/*
 * 获取连接事件epoll_event
 *
 */
uint32_t Channel::GetEvents() const
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    return events_;
}

/*
 * 设置读事件（EPOLLIN）处理函数
 *
 */
void Channel::SetReadHandle(const Callback &cb)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    readHandler_ = cb;
}

/*
 * 设置写事件（EPOLLOUT）处理函数
 *
 */
void Channel::SetWriteHandle(const Callback &cb)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    writeHandler = cb;
}

/*
 * 设置出错处理函数
 *
 */
void Channel::SetErrorHandle(const Callback &cb)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    errorHandler_ = cb;
}

/*
 * 设置连接关闭函数
 *
 */
void Channel::SetCloseHandle(const Callback &cb)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    closeHandler_ = cb;
}

/*
 * 执行连接事件
 *
 */
void Channel::HandleEvent()
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd：%d\n", fd_);
    if (events_ & EPOLLRDHUP)
    {
        // 客户端异常关闭事件
        LOG(LoggerLevel::INFO, "客户端异常关闭（EPOLLRDHUP），sockfd：%d\n", fd_);
        closeHandler_();
    }
    else if (events_ & (EPOLLIN | EPOLLPRI))
    {
        // 读事件，客户端有数据或者正常关闭
        LOG(LoggerLevel::INFO, "客户端有数据可读或正常关闭（EPOLLIN | EPOLLPRI），sockfd：%d\n", fd_);
        // std::cout << "Channel::HandleEvent 读取客户端的请求数据，sockfd：" << fd_ << std::endl;
        readHandler_();
    }
    else if (events_ & EPOLLOUT)
    {
        // 写事件，发送数据到客户端
        LOG(LoggerLevel::INFO, "客户端请求获取数据（EPOLLOUT），sockfd：%d\n", fd_);
        // std::cout << "Channel::HandleEvent 客户端请求获取数据，sockfd：" << fd_ << std::endl;
        writeHandler();
    }
    else
    {
        // 连接错误
        LOG(LoggerLevel::INFO, "客户端连接错误，sockfd：%d\n", fd_);
        std::cout << "Channel::HandleEvent 连接错误，sockfd：" << fd_ << std::endl;
        errorHandler_();
    }
}
