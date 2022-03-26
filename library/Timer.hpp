
// Timer类
//  定时器，生命周期由用户自行管理

#pragma once

#include <functional>
#include <sys/time.h>
// #include "TimerManager.hpp"

class TimerManager;

class Timer
{
public:
    typedef std::function<void()> CallBack;
    typedef enum
    {
        TIMER_ONCE = 0, // 单次触发
        TIMER_PERIOD    // 无限循环
    } TimerType;
    Timer(int timeout, TimerType timertype, const CallBack &timerCallBack);
    ~Timer();
    int timeOut_;           // 超时时间
    TimerType timerType_;   // 定时器类型
    CallBack timerCallBack_;// 触发函数
    int rotation;   // 定时器仍需等待的轮数
    int timeSlot;   // 
    Timer *prev;    // 
    Timer *next;    // 
    void Start();   // 
    void Stop();    // 
    void Adjust(int timeout, Timer::TimerType timertype, const CallBack &timerCallBack);    // 重新设置定时器

};

Timer::Timer(int timeout, TimerType timertype, const CallBack &timercallback)
    : timeOut_(timeout),
      timerType_(timertype),
      timerCallBack_(timercallback),
      rotation(0),
      timeSlot(0),
      prev(nullptr),
      next(nullptr)
{
    if (timeout < 0)
        return;
}

Timer::~Timer()
{
    Stop();
}

/*
 * 启动定时器
 * 
 */
void Timer::Start()
{
    // TimerManager::GetTimerManagerInstance()->AddTimer(this);
}

/*
 * 停止定时器
 * 
 */
void Timer::Stop()
{
    // TimerManager::GetTimerManagerInstance()->RemoveTimer(this);
}

/*
 * 更新定时器信息
 * 超时时间、定时器类型、触发函数
 * 
 */
void Timer::Adjust(int timeOut, Timer::TimerType timerType, const CallBack &timerCallBack)
{
    timeOut_ = timeOut;
    timerType_ = timerType;
    timerCallBack_ = timerCallBack;
    // TimerManager::GetTimerManagerInstance()->AdjustTimer(this);
}
