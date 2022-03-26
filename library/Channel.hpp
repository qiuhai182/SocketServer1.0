
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
//              这是相对于水平触发(Level Triggered)来说的；
//  EPOLLONESHOT：  只监听一次事件，当监听完这次事件之后，
//                  如果还需要继续监听这个socket的话，
//                  需要再次把这个socket加入到EPOLL队列里；

#pragma once

#include <functional>
#include <iostream>
#include <sys/epoll.h>

class Channel
{
public:
    typedef std::function<void()> Callback;
    Channel();
    ~Channel();
    void SetFd(int fd);     // 设置连接套接字fd
    int GetFd() const;      // 获取连接套接字fd
    void SetEvents(uint32_t events);    // 设置连接监听事件epoll_event
    uint32_t GetEvents() const;         // 获取连接事件epoll_event
    void SetReadHandle(const Callback &cb); //设置数据读取函数
    void SetWriteHandle(const Callback &cb);// 设置写事件函数
    void SetErrorHandle(const Callback &cb);// 设置出错处理函数
    void SetCloseHandle(const Callback &cb);// 设置连接关闭函数
    void HandleEvent();     // 执行连接事件

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
}

Channel::~Channel()
{
}

/*
 * 设置连接套接字fd
 * 
 */
void Channel::SetFd(int fd)
{
    fd_ = fd;
}

/*
 * 获取连接套接字fd
 * 
 */
int Channel::GetFd() const
{
    return fd_;
}

/*
 * 设置连接监听事件epoll_event
 * 
 */
void Channel::SetEvents(uint32_t events)
{
    events_ = events;
}

/*
 * 获取连接事件epoll_event
 * 
 */
uint32_t Channel::GetEvents() const
{
    return events_;
}

/*
 * 设置数据读取函数
 * 
 */
void Channel::SetReadHandle(const Callback &cb)
{
    readHandler_ = cb;
}

/*
 * 设置写事件函数
 * 
 */
void Channel::SetWriteHandle(const Callback &cb)
{
    writeHandler = cb;
}

/*
 * 设置出错处理函数
 * 
 */
void Channel::SetErrorHandle(const Callback &cb)
{
    errorHandler_ = cb;
}

/*
 * 设置连接关闭函数
 * 
 */
void Channel::SetCloseHandle(const Callback &cb)
{
    closeHandler_ = cb;
}

/*
 * 执行连接事件
 * 
 */
void Channel::HandleEvent()
{
    if (events_ & EPOLLRDHUP) // 客户端异常关闭事件
    {
        std::cout << "Event EPOLLRDHUP" << std::endl;
        closeHandler_();
    }
    else if (events_ & (EPOLLIN | EPOLLPRI)) // 读事件，客户端有数据或者正常关闭
    {
        readHandler_();
    }
    else if (events_ & EPOLLOUT) // 写事件
    {
        writeHandler();
    }
    else // 连接错误
    {
        errorHandler_();
    }
}
