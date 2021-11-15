
// TimerManager类，定时器管理，基于时间轮实现，增加删除O(1),执行可能复杂度高些，slot多的话可以降低链表长度

#ifndef _TIMER_MANAGER_H_
#define _TIMER_MANAGER_H_

#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include "Timer.h"

class TimerManager
{
public: 
    typedef std::function<void()> CallBack;

    // 单例模式
    static TimerManager* GetTimerManagerInstance(); 

    // 添加定时任务
    void AddTimer(Timer* ptimer); 

    // 删除定时任务
    void RemoveTimer(Timer* ptimer); 

    // 调整定时任务
    void AdjustTimer(Timer* ptimer); 

    // 开启线程，定时器启动
    void Start(); 

    // 暂停定时器
    void Stop(); 

    // 垃圾回收，程序结束的时候析构TimerManager
    class GC
    {
        public:
            ~GC()
            {
                if(timerManager_ != nullptr)
                    delete timerManager_;
            }
    };

private:
    // 私有构造函数，单例模式
    TimerManager();
    ~TimerManager();

    static TimerManager *timerManager_;
    static std::mutex mutex_;
    static GC gc;

    // 时间轮结构
    std::vector<Timer*> timeWheel; 

    // 时间轮互斥量
    std::mutex timeWheelMutex_; 

    // 时间轮运行状态
    bool running_; 

    // 时间轮当前slot
    int currentSlot; 

    // 每个slot的时间间隔,ms
    static const int slotInterval; 

    // slot总数
    static const int slotNum; 

    // 定时器线程
    std::thread th_; 

    // 时间轮检查超时任务
    void CheckTimer(); 

    // 线程实际执行的函数
    void CheckTick(); 

    // 计算定时器参数
    void CalculateTimer(Timer* ptimer); 

    // 添加定时器到时间轮中
    void AddTimerToTimeWheel(Timer* ptimer); 

    // 从时间轮中移除定时器
    void RemoveTimerFromTimeWheel(Timer* ptimer); 

    // 从时间轮中修改定时器
    void AdjustTimerToWheel(Timer* ptimer); 
};

#endif
