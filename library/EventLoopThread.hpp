
// EventLoopThread类：
//  特殊任务线程

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <thread> 
#include "EventLoop.hpp"

class EventLoopThread
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* GetLoop();
    void ThreadFunc();
    void Start();

private:
    std::thread childThread_;
    std::thread::id curThreadId_;
    std::string threadName_;
    EventLoop *loop_;

};

EventLoopThread::EventLoopThread()
    : childThread_(),
      curThreadId_(-1),
      threadName_("IO thread "),
      loop_(NULL)
{
}

EventLoopThread::~EventLoopThread()
{
    loop_->Quit();
    // 析构时同步启动子线程
    // 清理IO线程，防止内存泄漏，因为pthread_created会calloc
    childThread_.join();
}

/*
 * 
 * 
 */
EventLoop *EventLoopThread::GetLoop()
{
    return loop_;
}

/*
 * 创建子线程
 * 
 */
void EventLoopThread::Start()
{
    childThread_ = std::thread(&EventLoopThread::ThreadFunc, this);
}

/*
 * 子线程回调函数
 * 
 */
void EventLoopThread::ThreadFunc()
{
    // 子线程内部
    EventLoop loop;
    loop_ = &loop;
    curThreadId_ = std::this_thread::get_id();
    std::stringstream sin;
    sin << curThreadId_;
    threadName_ += sin.str();
    try
    {
        loop_->loop();
    }
    catch (std::bad_alloc &ba)
    {
        std::cerr << "bad_alloc caught in EventLoopThread::ThreadFunc loop: " << ba.what() << '\n';
    }
}


