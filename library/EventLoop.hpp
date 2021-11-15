
// 事件处理主逻辑，IO复用流程的抽象，等待、处理事件

#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include "Poller.hpp"
#include "Channel.hpp"

class EventLoop
{
public:
    typedef std::function<void()> Functor;
    typedef std::vector<Channel *> ChannelList;
    EventLoop();
    ~EventLoop();
    void loop();
    void AddChannelToPoller(Channel *pchannel)
    {
        poller_.AddChannel(pchannel);
    }
    void RemoveChannelToPoller(Channel *pchannel)
    {
        poller_.RemoveChannel(pchannel);
    }
    void UpdateChannelToPoller(Channel *pchannel)
    {
        poller_.UpdateChannel(pchannel);
    }
    void Quit()
    {
        quit_ = true;
    }
    std::thread::id GetThreadId() const
    {
        return tid_;
    }
    void AddTask(Functor functor)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            functorList_.push_back(functor);
        }
        WakeUp(); // 跨线程唤醒，worker线程唤醒IO线程
    }
    void WakeUp();
    void HandleRead();
    void HandleError();
    void ExecuteTask();

private:
    std::vector<Functor> functorList_;
    ChannelList activeChannelList_;
    Poller poller_;
    bool quit_;
    std::thread::id tid_;
    std::mutex mutex_;
    int wakeUpFd_;
    Channel wakeUpChannel_;
};

/*
 * 参照muduo，实现跨线程唤醒
 * 
 */
int CreateEventFd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        std::cout << "Failed in eventfd" << std::endl;
        exit(1);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : functorList_(),
      activeChannelList_(),
      poller_(),
      quit_(true),
      tid_(std::this_thread::get_id()),
      mutex_(),
      wakeUpFd_(CreateEventFd()),
      wakeUpChannel_()
{
    // wakeUpChannel_.SetFd(wakeUpFd_);
    // wakeUpChannel_.SetEvents(EPOLLIN | EPOLLET);
    // wakeUpChannel_.SetReadHandle(std::bind(&EventLoop::HandleRead, this));
    // wakeUpChannel_.SetErrorHandle(std::bind(&EventLoop::HandleError, this));
    // AddChannelToPoller(&wakeUpChannel_);
}

EventLoop::~EventLoop()
{
    close(wakeUpFd_);
}

/*
 * 
 * 
 */
void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeUpFd_, &one, sizeof one);
}

/*
 * 
 * 
 */
void EventLoop::HandleError()
{
}

/*
 * 
 * 
 */
void EventLoop::WakeUp()
{
    uint64_t one = 1;
    ssize_t n = write(wakeUpFd_, (char *)(&one), sizeof one);
}

/*
 * 
 * 
 */
void EventLoop::loop()
{
    quit_ = false;
    while (!quit_)
    {
        poller_.poll(activeChannelList_);
        for (Channel *pchannel : activeChannelList_)
        {
            std::cout << "one connection, fd = " << pchannel->GetFd() << std::endl;
            pchannel->HandleEvent();
        }
        activeChannelList_.clear();
        if (functorList_.size())
        {
            ExecuteTask();
        }
    }
}

/*
 * 
 * 
 */
void EventLoop::ExecuteTask()
{
    //  std::lock_guard <std::mutex> lock(mutex_);
    //  for(Functor &functor : functorList_)
    //  {
    //      functor();// 在加锁后执行任务，调用sendinloop，再调用close，执行添加任务，这样functorList_就会修改
    //  }
    //  functorList_.clear();
    std::vector<Functor> functorlists;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functorlists.swap(functorList_);
    }
    for (Functor &functor : functorlists)
    {
        functor();
    }
    functorlists.clear();
}
