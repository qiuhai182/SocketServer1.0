
//EventLoopThread类，表示IO线程,执行特定任务的,线程池的是通用任务线程

#ifndef _EVENTLOOP_THREAD_POOL_H_
#define _EVENTLOOP_THREAD_POOL_H_

#include <iostream>
#include <vector>
#include <string>
//#include <queue>
//#include <mutex>  
//#include <condition_variable>
#include "EventLoop.h"
#include "EventLoopThread.h"

class EventLoopThreadPool
{
private:
    //主loop
    EventLoop *mainloop_;

    //线程数量
    int threadnum_;

    //线程列表
    std::vector<EventLoopThread*> threadlist_;

    //用于轮询分发的索引
    int index_;

public:
    EventLoopThreadPool(EventLoop *mainloop, int threadnum = 0);
    ~EventLoopThreadPool();

    //启动线程池
    void Start();

    //获取下一个被分发的loop，依据RR轮询策略
    EventLoop* GetNextLoop();
};

#endif
