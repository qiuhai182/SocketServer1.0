
// EventLoopThread类，通用任务线程

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "EventLoop.hpp"
#include "EventLoopThread.hpp"

class EventLoopThreadPool
{
private:
    std::vector<EventLoopThread*> eventLoopThreadList_;
    EventLoop *mainLoop_;
    int threadNum_;
    int index_; // 用于轮询分发的索引

public:
    EventLoopThreadPool(EventLoop *mainloop, int threadnum = 0);
    ~EventLoopThreadPool();
    void Start();
    EventLoop* GetNextLoop();

};

EventLoopThreadPool::EventLoopThreadPool(EventLoop *mainloop, int threadNum)
    : mainLoop_(mainloop),
      threadNum_(threadNum),
      eventLoopThreadList_(),
      index_(0)
{
    for (int i = 0; i < threadNum_; ++i)
    {
        EventLoopThread *preLoopThread = new EventLoopThread;
        eventLoopThreadList_.push_back(preLoopThread);
    }
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    for (int i = 0; i < threadNum_; ++i)
    {
        delete eventLoopThreadList_[i];
    }
    eventLoopThreadList_.clear();
}

void EventLoopThreadPool::Start()
{
    if (threadNum_ > 0)
    {
        for (int i = 0; i < threadNum_; ++i)
        {
            eventLoopThreadList_[i]->Start();
        }
    }
}

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
