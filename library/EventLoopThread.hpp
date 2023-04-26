
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
    // 线程实体构造完成即在运行
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
    childThread_ = std::thread(&EventLoopThread::ThreadFunc, this);
}

EventLoopThread::~EventLoopThread()
{
    LOG(LoggerLevel::INFO, "函数触发，curThread: %d\n", curThreadId_);
    loop_->Quit();
    // join函数该清理IO线程，防止内存泄漏，因为pthread_created会calloc
    childThread_.join();
}

/*
 * 获取事件线程的事件池对象指针
 * 
 */
EventLoop *EventLoopThread::GetLoop()
{
    LOG(LoggerLevel::INFO, "函数触发，curThread: %d\n", curThreadId_);
    return loop_;
}

/*
 * 子线程回调函数
 * 
 */
void EventLoopThread::ThreadFunc()
{
    // 子线程内部
    LOG(LoggerLevel::INFO, "函数触发，回调函数可能位于子线程内，curThread: %d\n", curThreadId_);
    EventLoop loop;
    loop_ = &loop;
    curThreadId_ = std::this_thread::get_id();
    std::stringstream sin;
    sin << curThreadId_;
    threadName_ += sin.str();
    // std::cout << "EventLoopThread::ThreadFunc 事件池工作线程：" << threadName_ << " 启动" << std::endl;
    LOG(LoggerLevel::INFO, "事件池工作线程启动，curThread：%d，threadName：%s\n", curThreadId_, threadName_);
    try
    {
        // 启动事件池循环监听
        loop_->loop();
    }
    catch (std::bad_alloc &ba)
    {
        std::cerr << "EventLoopThread::ThreadFunc bad_alloc caught in EventLoopThread::ThreadFunc loop: " << ba.what() << '\n';
    }
    std::cout << "EventLoopThread::ThreadFunc 事件池工作线程" << threadName_ << " 退出" << std::endl;
    LOG(LoggerLevel::INFO, "事件池工作线程退出，curThread：%d，threadName：%s\n", curThreadId_, threadName_);
}
