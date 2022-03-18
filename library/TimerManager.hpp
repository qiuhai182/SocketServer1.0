
// TimerManager类
//  定时器管理类，基于时间轮实现，增加删除O(1)，执行可能复杂度高些，slot多的话可以降低链表长度

#pragma once

#include <iostream>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <ctime>
#include <ratio>
#include <chrono>
#include <unistd.h>
#include <sys/time.h>
#include "Timer.hpp"

class TimerManager
{
public:
    typedef std::function<void()> CallBack;
    static TimerManager *GetTimerManagerInstance(); // 获取TimerManager单例指针
    void AddTimer(Timer *ptimer);
    void RemoveTimer(Timer *ptimer);
    void AdjustTimer(Timer *ptimer);
    void Start();
    void Stop();
    class GC
    {
    public:
        ~GC()
        {
            if (timerManager_ != nullptr)
                delete timerManager_;
        }
    };

private:
    TimerManager();     // 单例模式
    ~TimerManager();
    static TimerManager *timerManager_;
    static std::mutex mutex_;
    static GC gc;
    std::vector<Timer *> timeWheel;
    std::mutex timeWheelMutex_;
    bool running_;
    int currentSlot;
    static const int slotInterval;
    static const int slotNum;
    std::thread th_;
    void CheckTimer();
    void CheckTick();
    void CalculateTimer(Timer *ptimer);
    void AddTimerToTimeWheel(Timer *ptimer);
    void RemoveTimerFromTimeWheel(Timer *ptimer);
    void AdjustTimerToWheel(Timer *ptimer);

};

// 全局初始化（静态初始化、常量初始化）
std::mutex TimerManager::mutex_;
TimerManager *TimerManager::timerManager_ = nullptr;    // 
TimerManager::GC TimerManager::gc;          // 
const int TimerManager::slotInterval = 1;   // 
const int TimerManager::slotNum = 1024;     // 最大定时器数量

TimerManager::TimerManager()
    : currentSlot(0),
      timeWheel(slotNum, nullptr),
      running_(false),
      th_()
{
}

TimerManager::~TimerManager()
{
    Stop();
}

/*
 * 获取TimerManager单例指针
 * 
 */
TimerManager *TimerManager::GetTimerManagerInstance()
{
    if (timerManager_ == nullptr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (timerManager_ == nullptr)
        {
            timerManager_ = new TimerManager();
        }
    }
    return timerManager_;
}

/*
 * 添加一个定时器任务到定时器列表timeWheel（时间轮）
 * 实际传入需要计算Timer指针的参数
 */
void TimerManager::AddTimer(Timer *ptimer)
{
    if (ptimer == nullptr)
        return;
    std::lock_guard<std::mutex> lock(timeWheelMutex_);
    CalculateTimer(ptimer);
    AddTimerToTimeWheel(ptimer);
}

/*
 * 添加一个定时器任务到定时器列表timeWheel
 * 
 */
void TimerManager::AddTimerToTimeWheel(Timer *ptimer)
{
    if (ptimer == nullptr)
        return;
    int timeslot = ptimer->timeSlot;
    if (timeWheel[timeslot])
    {
        ptimer->next = timeWheel[timeslot];
        timeWheel[timeslot]->prev = ptimer;
        timeWheel[timeslot] = ptimer;
    }
    else
    {
        timeWheel[timeslot] = ptimer;
    }
}

/*
 * 从定时器列表timeWheel删除一个定时任务
 * 
 */
void TimerManager::RemoveTimer(Timer *ptimer)
{
    if (ptimer == nullptr)
        return;
    std::lock_guard<std::mutex> lock(timeWheelMutex_);
    RemoveTimerFromTimeWheel(ptimer);
}

/*
 * 从定时器列表timeWheel删除一个定时任务
 * 
 */
void TimerManager::RemoveTimerFromTimeWheel(Timer *ptimer)
{
    if (ptimer == nullptr)
        return;
    int timeslot = ptimer->timeSlot;
    if (ptimer == timeWheel[timeslot])
    {
        // 头结点
        timeWheel[timeslot] = ptimer->next;
        if (ptimer->next != nullptr)
        {
            ptimer->next->prev = nullptr;
        }
        ptimer->prev = ptimer->next = nullptr;
    }
    else
    {
        if (ptimer->prev == nullptr) // 不在时间轮的链表中，即已经被删除了
            return;
        ptimer->prev->next = ptimer->next;
        if (ptimer->next != nullptr)
            ptimer->next->prev = ptimer->prev;
        ptimer->prev = ptimer->next = nullptr;
    }
}

/*
 * 修改定时器列表timeWheel内一个定时任务的信息
 * 
 */
void TimerManager::AdjustTimer(Timer *ptimer)
{
    if (ptimer == nullptr)
        return;
    std::lock_guard<std::mutex> lock(timeWheelMutex_);
    AdjustTimerToWheel(ptimer);
}

/*
 * 修改定时器列表timeWheel内一个定时任务的信息
 * 
 */
void TimerManager::AdjustTimerToWheel(Timer *ptimer)
{
    if (ptimer == nullptr)
        return;
    RemoveTimerFromTimeWheel(ptimer);
    CalculateTimer(ptimer);
    AddTimerToTimeWheel(ptimer);
}

/*
 * 计算定时器参数
 * 
 */
void TimerManager::CalculateTimer(Timer *ptimer)
{
    if (ptimer == nullptr)
        return;
    int tick = 0;
    int timeout = ptimer->timeOut_;
    if (timeout < slotInterval)
    {
        tick = 1;
    }
    else
    {
        tick = timeout / slotInterval;
    }
    ptimer->rotation = tick / slotNum;
    int timeslot = (currentSlot + tick) % slotNum;
    ptimer->timeSlot = timeslot;
}

/*
 * 时间轮检查超时任务
 * 
 */
void TimerManager::CheckTimer() // 执行当前slot的任务
{
    std::lock_guard<std::mutex> lock(timeWheelMutex_);
    Timer *ptimer = timeWheel[currentSlot];
    while (ptimer != nullptr)
    {
        if (ptimer->rotation > 0)
        {
            --ptimer->rotation;
            ptimer = ptimer->next;
        }
        else
        {
            // 可执行定时器任务
            ptimer->timerCallBack_(); // 注意：任务里不能把定时器自身给清理掉！！！我认为应该先移除再执行任务
            if (ptimer->timerType_ == Timer::TimerType::TIMER_ONCE)
            {
                Timer *ptemptimer = ptimer;
                ptimer = ptimer->next;
                RemoveTimerFromTimeWheel(ptemptimer);
            }
            else
            {
                Timer *ptemptimer = ptimer;
                ptimer = ptimer->next;
                AdjustTimerToWheel(ptemptimer);
                if (currentSlot == ptemptimer->timeSlot && ptemptimer->rotation > 0)
                {
                    // 如果定时器多于一转的话，需要先对rotation减1，否则会等待两个周期
                    --ptemptimer->rotation;
                }
            }
        }
    }
    currentSlot = (++currentSlot) % TimerManager::slotNum; // 移动至下一个时间槽
}

/*
 * 线程实际执行任务的函数
 * 
 */
void TimerManager::CheckTick()
{
    //  steady_clock::time_point t1 = steady_clock::now();
    //  steady_clock::time_point t2 = steady_clock::now();
    //  duration<double> time_span;
    int si = TimerManager::slotInterval;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int oldtime = (tv.tv_sec % 10000) * 1000 + tv.tv_usec / 1000;
    std::cout << "oldtime: " << oldtime << ", sec: " << tv.tv_sec << ", usec: " << tv.tv_usec << std::endl;
    int time;
    int tickcount;
    while (running_)
    {
        gettimeofday(&tv, NULL);
        time = (tv.tv_sec % 10000) * 1000 + tv.tv_usec / 1000;
        tickcount = (time - oldtime) / slotInterval; // 计算两次check的时间间隔占多少个slot
        // oldtime = time;
        oldtime = oldtime + tickcount * slotInterval;
        for (int i = 0; i < tickcount; ++i)
        {
            TimerManager::GetTimerManagerInstance()->CheckTimer();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500)); // milliseconds(si)时间粒度越小，延时越不精确，因为上下文切换耗时
        //  t2 = steady_clock::now();
        //  time_span = duration_cast<duration<double>>(t2 - t1);
        //  t1 = t2;
        //  std::cout << "thread " << time_span.count() << " seconds." << std::endl;
    }
}

/*
 * 启动TimerManager线程
 * 
 */
void TimerManager::Start()
{
    running_ = true;
    th_ = std::thread(&TimerManager::CheckTick, this);
}

/*
 * 回收TimerManager线程
 * 
 */
void TimerManager::Stop()
{
    running_ = false;
    if (th_.joinable())
        th_.join();
}
