
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
    void AddChannelToPoller(Channel *pchannel);
    void RemoveChannelToPoller(Channel *pchannel);
    void UpdateChannelToPoller(Channel *pchannel);
    void Quit();
    std::thread::id GetThreadId() const;
    void AddTask(Functor functor);
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
 * eventfd函数创建文件描述符
 * 进程/线程间共享同一个内核维护uint值存取，支持read、write
 */
int CreateEventFd()
{
    int evtFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtFd < 0)
    {
        std::cout << "Failed in eventfd" << std::endl;
        exit(1);
    }
    return evtFd;
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
 * 唤醒线程读取数据函数
 * 
 */
void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeUpFd_, &one, sizeof one);
}

/*
 * 唤醒线程错误处理函数
 * 
 */
void EventLoop::HandleError()
{
}

/*
 * Poller监听Channel对应新连接
 * 
 */
void EventLoop::AddChannelToPoller(Channel *pchannel)
{
    poller_.AddChannel(pchannel);
}

/*
 * Poller移除Channel对应连接监听
 * 
 */
void EventLoop::RemoveChannelToPoller(Channel *pchannel)
{
    poller_.RemoveChannel(pchannel);
}

/*
 * Poller更改Channel对应连接事件信息
 * 
 */
void EventLoop::UpdateChannelToPoller(Channel *pchannel)
{
    poller_.UpdateChannel(pchannel);
}

/*
 * 停止运行EventLoop事件循环
 * 
 */
void EventLoop::Quit()
{
    quit_ = true;
}

/*
 * 获取EventLoop所在线程ID
 * 
 */
std::thread::id EventLoop::GetThreadId() const
{
    return tid_;
}

/*
 * EventLoop添加任务函数到functorList_
 * 唤醒工作线程
 */
void EventLoop::AddTask(Functor functor)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functorList_.push_back(functor);
    }
    WakeUp();
}

/*
 * 唤醒线程，实则为向eventfd生成的文件描述符内写入唤醒标志值
 * 
 */
void EventLoop::WakeUp()
{
    uint64_t one = 1;
    ssize_t n = write(wakeUpFd_, (char *)(&one), sizeof one);
}

/*
 * EventLoop循环函数，循环调用Poller封装poll函数，获取一批次新连接
 * 调用每一个连接对应的Channel的HandleEvent函数，执行动态绑定的处理函数
 * 执行functorList_里的所有任务函数
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
 * 执行functorList_里的所有任务函数
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
