
//Channel类，fd和事件的封装

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <functional>

class Channel
{
public:
    //回调函数类型
    typedef std::function<void()> Callback;

    Channel();
    ~Channel();

    //设置文件描述符
    void SetFd(int fd) 
    {
        fd_ = fd; 
    }

    //获取文件描述符
    int GetFd() const
    { 
        return fd_; 
    }    

    //设置触发事件
    void SetEvents(uint32_t events)
    { 
        events_ = events; 
    }

    //获取触发事件
    uint32_t GetEvents() const
    { 
        return events_; 
    }

    //事件分发处理
    void HandleEvent();

    //设置读事件回调
    void SetReadHandle(const Callback &cb)
    {
        readhandler_ = cb; //提高效率，可以使用move语义,这里暂时还是存在一次拷贝
    }

    //设置写事件回调
    void SetWriteHandle(const Callback &cb)
    {
        writehandler_ = cb; 
    }    

    //设置错误事件回调
    void SetErrorHandle(const Callback &cb)
    { 
        errorhandler_ = cb;
    }

    //设置close事件回调
    void SetCloseHandle(const Callback &cb)
    {
        closehandler_ = cb;
    }

private:
    //文件描述符
    int fd_;
    //事件，一般情况下为epoll events 
    uint32_t events_;

    //事件触发时执行的函数，在tcpconn中注册
    Callback readhandler_;
    Callback writehandler_;
    Callback errorhandler_;
    Callback closehandler_;
};

#endif