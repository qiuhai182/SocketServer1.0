
// Timer类，定时器

#include "Timer.h"
#include <sys/time.h>
#include "TimerManager.h"

Timer::Timer(int timeout, TimerType timertype, const CallBack &timercallback)
    : timeOut_(timeout),
    timerType_(timertype),
    timerCallBack_(timercallback),
    rotation(0),
    timeSlot(0),
    prev(nullptr),
    next(nullptr)
{
    if(timeout < 0)
        return;
}

Timer::~Timer()
{
    Stop();
}

void Timer::Start()
{
    TimerManager::GetTimerManagerInstance()->AddTimer(this);
}

void Timer::Stop()
{
    TimerManager::GetTimerManagerInstance()->RemoveTimer(this);
}

void Timer::Adjust(int timeOut, Timer::TimerType timerType, const CallBack &timerCallBack)
{
    timeOut_ = timeOut;
    timerType_ = timerType;
    timerCallBack_ = timerCallBack;
    TimerManager::GetTimerManagerInstance()->AdjustTimer(this);
}
