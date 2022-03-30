
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
    EventLoop* GetLoop();   // 获取事件线程的事件池对象指针
    void ThreadFunc();      // 工作线程的回调函数

private:
    std::thread childThread_;       // 工作子线程
    std::thread::id curThreadId_;   // 当前线程ID，使用时由子线程内部为其赋值
    std::string threadName_;        // 线程名，使用时由子线程内部为其补全赋值
    EventLoop *loop_;               // 事件池实例对象

};

EventLoopThread::EventLoopThread()
    : childThread_(),
      curThreadId_(-1),
      threadName_(""),
      loop_(NULL)
{
    childThread_ = std::thread(&EventLoopThread::ThreadFunc, this);
}

EventLoopThread::~EventLoopThread()
{
    loop_->Quit();
    // 析构时同步启动子线程
    // 清理IO线程，防止内存泄漏，因为pthread_created会calloc
    childThread_.join();
}

/*
 * 获取事件线程的事件池对象指针
 * 
 */
EventLoop *EventLoopThread::GetLoop()
{
    return loop_;
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
    std::cout << "事件池工作线程：" << threadName_ << " 启动" << std::endl;
    try
    {
        // 启动事件池循环监听
        loop_->loop();
    }
    catch (std::bad_alloc &ba)
    {
        std::cerr << "bad_alloc caught in EventLoopThread::ThreadFunc loop: " << ba.what() << '\n';
    }
    std::cout << "事件池工作线程" << threadName_ << " 退出" << std::endl;
}


