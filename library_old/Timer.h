
// Timer类，定时器，生命周期由用户自行管理

#ifndef _TIMER_H_
#define _TIMER_H_

#include <functional>

class Timer
{
public:
    // 定时器回调函数
    typedef std::function<void()> CallBack;

    // 定时器类型
    typedef enum
    {
        TIMER_ONCE = 0, // 单次触发
        TIMER_PERIOD // 无限循环
    }TimerType;

    // 超时时间,单位ms
    int timeOut_; 

    // 定时器类型
    TimerType timerType_; 

    // 回调函数
    CallBack timerCallBack_; 

    // 定时器剩下的转数
    int rotation; 

    // 定时器所在的时间槽
    int timeSlot; 

    // 定时器链表指针
    Timer *prev;
    Timer *next;

    Timer(int timeout, TimerType timertype, const CallBack &timerCallBack);
    ~Timer();

    // 定时器启动，加入管理器
    void Start(); 

    // 定时器撤销，从管理器中删除
    void Stop(); 

    // 重新设置定时器
    void Adjust(int timeout, Timer::TimerType timertype, const CallBack &timerCallBack); 

};

#endif