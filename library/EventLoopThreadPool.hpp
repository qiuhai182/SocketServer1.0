
// EventLoopThread类:
//  通用任务线程

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "EventLoop.hpp"
#include "EventLoopThread.hpp"

class EventLoopThreadPool
{
public:
    EventLoopThreadPool(EventLoop *mainloop, int threadnum = 0);
    ~EventLoopThreadPool();
    EventLoop* GetNextLoop();   // 轮询分发EventLoop指针

private:
    std::vector<EventLoopThread*> eventLoopThreadList_; // 任务线程实例列表
    EventLoop *mainLoop_;   // 事件池主逻辑控制实例
    int threadNum_;         // 任务线程数量
    int index_;             // 用于轮询分发的索引，根据此索引向外部提供
                            // 索引指向的任务线程类实例EventLoopThread所创建的EventLoop对象指针

};

EventLoopThreadPool::EventLoopThreadPool(EventLoop *mainloop, int threadNum)
    : mainLoop_(mainloop),
      threadNum_(threadNum),
      eventLoopThreadList_(),
      index_(0)
{
    // 创建threadNum个事件池工作线程
    for (int i = 0; i < threadNum_; ++i)
    {
        // 线程实体构造完成即在运行
        EventLoopThread *preLoopThread = new EventLoopThread;
        eventLoopThreadList_.push_back(preLoopThread);
    }
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // 删除每一个事件池线程实例
    for (int i = 0; i < threadNum_; ++i)
    {
        delete eventLoopThreadList_[i];
    }
    // 清空eventLoopThreadList_列表内所有已失效的子线程实例对象
    eventLoopThreadList_.clear();
}

/*
 * 轮询分发EventLoop指针
 * 
 */
EventLoop *EventLoopThreadPool::GetNextLoop()
{
    if (threadNum_ > 0)
    {
        EventLoop *loop = eventLoopThreadList_[index_]->GetLoop();
        index_ = (index_ + 1) % threadNum_;
        return loop;
    }
    else
    {
        return mainLoop_;
    }
}



