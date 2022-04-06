
// 事件监听并处理主逻辑：
//  创建Poller实例poller_并循环执行poll函数监听客户端连接
//  每一次循环将任务列表functorList_里的任务全部执行
//  可以只有一个EventLoop工作
//  也可以一个主要EventLoop，控制多个位于事件池子线程的EventLoop
//  事件池线程池属于TcpServer控管，不同于工作线程池
//  事件池多线程时，子线程轮询提供EventLoop指针用于添加监听新连接
//  只有一个时，主线程循环调用loop监听epoll直至结束
//  多个线程时，子线程循环调用loop直至子线程结束，主线程虽也循环调用loop专用于监听TceServer的Channel

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
    std::thread::id GetThreadId() const;           // 获取EventLoop所在线程ID
    void loop();                                   // 循环监听事件并处理，以及执行functorList_上的任务
    void AddTask(Functor functor);                 // 添加任务到事件列表functorList_，唤醒工作线程
    void WakeUp();                                 // 唤醒工作线程
    void Quit();                                   // 停止运行EventLoop事件循环
    void ExecuteTask();                            // 执行functorList_里的所有任务函数
    void AddChannelToPoller(Channel *pchannel);    // Poller监听Channel对应新连接
    void RemoveChannelToPoller(Channel *pchannel); // Poller移除Channel对应连接监听
    void UpdateChannelToPoller(Channel *pchannel); // Poller更改Channel对应连接事件信息

private:
    std::mutex mutex_;                 // 锁
    std::vector<Functor> functorList_; // 任务列表
    std::thread::id tid_;              // 当前线程id
    ChannelList activeChannelList_;    // 连接列表，存储当前批次事件的Channel实例
    Poller poller_;                    // epoll封装类实例
    bool quit_;                        // 停止循环监听事件标志位
    int wakeUpFd_;                     // 共享内存fd，用于唤醒线程
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
      wakeUpFd_(CreateEventFd())
{
    std::cout << "输出测试：EventLoop::EventLoop 创建一个EventLoop" << std::endl;
}

EventLoop::~EventLoop()
{
    close(wakeUpFd_);
}

/*
 * Poller监听Channel对应新连接
 *
 */
void EventLoop::AddChannelToPoller(Channel *pchannel)
{
    std::cout << "输出测试：EventLoop::AddChannelToPoller 一个EventLoop添加连接监听，目标sockfd：" << pchannel->GetFd() << std::endl;
    // std::lock_guard<std::mutex> lock(mutex_);
    poller_.AddChannel(pchannel);
}

/*
 * Poller移除Channel对应连接监听
 *
 */
void EventLoop::RemoveChannelToPoller(Channel *pchannel)
{
    std::cout << "输出测试：EventLoop::RemoveChannelToPoller 一个EventLoop移除连接监听，目标sockfd：" << pchannel->GetFd() << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    poller_.RemoveChannel(pchannel);
}

/*
 * Poller更改Channel对应连接事件信息
 *
 */
void EventLoop::UpdateChannelToPoller(Channel *pchannel)
{
    std::cout << "输出测试：EventLoop::UpdateChannelToPoller 一个EventLoop更新连接监听，目标sockfd：" << pchannel->GetFd() << std::endl;
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
    std::cout << "输出测试：EventLoop::AddTask 添加一个待执行函数到事件池等待执行" << std::endl;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functorList_.push_back(functor);
    }
}

/*
 * 唤醒线程
 * 实则为向eventfd生成的文件描述符wakeUpFd_写入唤醒标志值
 * 此函数已废弃
 *
 */
void EventLoop::WakeUp()
{
    std::cout << "输出测试：EventLoop::WakeUp " << std::endl;
    uint64_t one = 1;
    std::lock_guard<std::mutex> lock(mutex_);
    ssize_t n = write(wakeUpFd_, (char *)(&one), sizeof one);
    /* 读取唤醒标志方式：
        uint64_t one = 1;
        std::lock_guard<std::mutex> lock(mutex_);
        ssize_t n = read(wakeUpFd_, &one, sizeof one);
        */
}

/*
 * 执行functorList_里的所有任务函数
 * 此函数在loop所在线程执行任务
 * 若当前loop线程是子线程，则类似于多线程任务函数
 *
 */
void EventLoop::ExecuteTask()
{
    std::cout << "输出测试：EventLoop::ExecuteTask " << std::endl;
    // 拷贝任务列表并使任务列表置空
    std::vector<Functor> functorlists;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functorlists.swap(functorList_);
    }
    // 执行拷贝的所有任务
    std::cout << "输出测试：EventLoop即将执行所在线程的IO任务，任务数量：" << functorlists.size() << std::endl;
    for (Functor &functor : functorlists)
    {
        std::cout << "输出测试：EventLoop即将执行一个IO任务" << std::endl;
        try
        {
            functor();
        }
        catch (std::bad_function_call)
        {
            std::cout << "输出测试：EventLoop执行一个IO任务报错：std::bad_function_call，函数调用失败" << std::endl;
        }
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
            std::cout << "输出测试：EventLoop::loop EventLoop处理新请求事件, 连接sockfd：" << pchannel->GetFd() << std::endl;
            pchannel->HandleEvent();
        }
        activeChannelList_.clear();
        if (functorList_.size())
        {
            ExecuteTask();
        }
    }
    std::cout << "输出测试：EventLoop::loop 一个事件池EventLoop退出" << std::endl;
}
