
// 事件监听并处理主逻辑：
//  负责循环执行poll函数监听客户端连接
//  可以只有一个EventLoop工作
//  也可以一个主要EventLoop，控制多个位于事件池子线程的EventLoop
//  事件池线程池属于TcpServer控管，不同于工作线程池
//  事件池多线程时，子线程轮询提供EventLoop指针用于添加监听新连接
//  只有一个时，主线程循环调用loop监听epoll直至结束
//  多个线程时，子线程循环调用loop直至子线程结束，主线程虽也循环调用loop却不监听任何连接

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
    void loop();    // 循环监听事件并处理，以及执行functorList_上的任务
    void AddChannelToPoller(Channel *pchannel);     // Poller监听Channel对应新连接
    void RemoveChannelToPoller(Channel *pchannel);  // Poller移除Channel对应连接监听
    void UpdateChannelToPoller(Channel *pchannel);  // Poller更改Channel对应连接事件信息
    void Quit();    // 停止运行EventLoop事件循环
    std::thread::id GetThreadId() const;    // 获取EventLoop所在线程ID
    void AddTask(Functor functor);          // 添加任务到事件列表functorList_，唤醒工作线程
    void WakeUp();      // 唤醒工作线程
    void HandleRead();  // 唤醒线程的读取数据函数
    void HandleError(); // 唤醒线程的错误处理函数
    void ExecuteTask(); // 执行functorList_里的所有任务函数

private:
    std::mutex mutex_;
    std::vector<Functor> functorList_;  // 任务列表
    ChannelList activeChannelList_;     // 连接列表，存储当前批次事件的Channel实例
    Poller poller_;                     // epoll封装类实例
    bool quit_;                         // 停止循环监听事件标志位
    std::thread::id tid_;               // 当前线程id
    int wakeUpFd_;                      // 共享内存fd，用于唤醒线程
    Channel wakeUpChannel_;

};

/*
 * eventfd函数创建文件描述符，用于进程/线程间通信
 * 共享同一个内核维护uint值存取，支持read、write
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
    std::cout << "创建一个EventLoop" << std::endl; 
}

EventLoop::~EventLoop()
{
    close(wakeUpFd_);
}

/*
 * 唤醒线程的读取数据函数
 * 
 */
void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeUpFd_, &one, sizeof one);
}

/*
 * 唤醒线程的错误处理函数
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
 * EventLoop添加任务到functorList_
 * 并唤醒工作线程
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
 * 唤醒线程
 * 实则为向eventfd生成的文件描述符wakeUpFd_
 * 写入唤醒标志值
 * 
 */
void EventLoop::WakeUp()
{
    uint64_t one = 1;
    ssize_t n = write(wakeUpFd_, (char *)(&one), sizeof one);
}

/*
 * 执行functorList_里的所有任务函数
 * 此函数在loop所在线程执行任务
 * 若当前loop线程是子线程，则类似于多线程任务函数
 * 
 */
void EventLoop::ExecuteTask()
{
    // 拷贝任务列表并使任务列表置空
    std::vector<Functor> functorlists;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functorlists.swap(functorList_);
    }
    // 执行拷贝的所有任务
    for (Functor &functor : functorlists)
    {
        functor();
    }
    functorlists.clear();
}

/*
 * EventLoop事件池循环监听并处理函数
 * 循环调用Poller封装poll函数，获取一批次新连接
 * 调用每一个连接对应的Channel的HandleEvent函数，执行动态绑定的处理函数
 * 并执行functorList_里的所有任务
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
            std::cout << "输出测试：EventLoop处理新连接, 连接sockfd：" << pchannel->GetFd() << std::endl;
            pchannel->HandleEvent();
        }
        activeChannelList_.clear();
        if (functorList_.size())
        {
            ExecuteTask();
        }
    }
    std::cout << "一个事件池EventLoop退出" << std::endl;
}



