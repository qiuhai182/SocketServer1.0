
// Channel类，fd和事件的封装
//  EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
//  EPOLLOUT：表示对应的文件描述符可以写；
//  EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
//  EPOLLERR：表示对应的文件描述符发生错误；
//  EPOLLHUP：表示对应的文件描述符被挂断；
//  EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
//  EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里

#pragma once

#include <functional>
#include <iostream>
#include <sys/epoll.h>

class Channel
{
public:
    // 回调函数
    typedef std::function<void()> Callback;
    Channel();
    ~Channel();
    void SetFd(int fd)
    {
        fd_ = fd;
    }
    int GetFd() const
    {
        return fd_;
    }
    void SetEvents(uint32_t events)
    {
        events_ = events;
    }
    uint32_t GetEvents() const
    {
        return events_;
    }
    void HandleEvent();
    void SetReadHandle(const Callback &cb)
    {
        readHandler_ = cb;
    }
    void SetWriteHandle(const Callback &cb)
    {
        writeHandler = cb;
    }
    void SetErrorHandle(const Callback &cb)
    {
        errorHandler_ = cb;
    }
    void SetCloseHandle(const Callback &cb)
    {
        closeHandler_ = cb;
    }

private:
    int fd_;
    uint32_t events_; // 事件，一般情况下为epoll_event.events
    Callback readHandler_;
    Callback writeHandler;
    Callback errorHandler_;
    Callback closeHandler_;

};

Channel::Channel()
    : fd_(-1)
{
}

Channel::~Channel()
{
}

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
