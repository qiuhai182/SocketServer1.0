
//ThreadPool类，简易线程池实现，表示worker线程,执行通用任务线程
//
// 使用的同步原语有 
// pthread_mutex_t mutex_l;//互斥锁
// pthread_cond_t condtion_l;//条件变量
// 使用的系统调用有
// pthread_mutex_init();
// pthread_cond_init();
// pthread_create(&thread_[i],NULL,threadFunc,this)
// pthread_mutex_lock()
// pthread_mutex_unlock()
// pthread_cond_signal()
// pthread_cond_wait()
// pthread_cond_broadcast();
// pthread_join()
// pthread_mutex_destory()
// pthread_cond_destory()

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <mutex>  
#include <thread> 
#include <condition_variable>
#include <functional>
#include <deque>
#include <unistd.h>

class ThreadPool
{
public:
    //线程池任务类型
    typedef std::function<void()> Task;

    ThreadPool(int threadnum = 0);
    ~ThreadPool();

    //启动线程池
    void Start();

    //暂停线程池
    void Stop();

    //添加任务
    void AddTask(Task task);

    //线程池执行的函数
    void ThreadFunc();

    //获取线程数量
    int GetThreadNum()
    { return threadnum_; }

private:
    //运行状态
    bool started_;

    //线程数量
    int threadnum_;

    //线程列表
    std::vector<std::thread*> threadlist_;

    //任务队列
    std::queue<Task> taskqueue_;

    //任务队列互斥量
    std::mutex mutex_;

    //任务队列同步的条件变量
    std::condition_variable condition_;
};

ThreadPool::ThreadPool(int threadnum)
    : started_(false),
      threadnum_(threadnum),
      threadlist_(),
      taskqueue_(),
      mutex_(),
      condition_()
{
}

ThreadPool::~ThreadPool()
{
    std::cout << "Clean up the ThreadPool " << std::endl;
    Stop();
    for (int i = 0; i < threadnum_; ++i)
    {
        threadlist_[i]->join();
    }
    for (int i = 0; i < threadnum_; ++i)
    {
        delete threadlist_[i];
    }
    threadlist_.clear();
}

void ThreadPool::Start()
{
    if (threadnum_ > 0)
    {
        started_ = true;
        for (int i = 0; i < threadnum_; ++i)
        {
            std::thread *pthread = new std::thread(&ThreadPool::ThreadFunc, this);
            threadlist_.push_back(pthread);
        }
    }
    else
    {
        ;
    }
}

void ThreadPool::Stop()
{
    started_ = false;
    condition_.notify_all();
}

void ThreadPool::AddTask(Task task)
{
    {
        // 创建即加锁、析构自动解锁
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.push(task);
    }
    // 解锁正在等待当前条件的线程中的一个，只有一个时即解锁该线程
    // 如果没有线程在等待，则函数不执行任何操作，
    // 如果正在等待的线程多余一个，则唤醒的线程是不确定的
    condition_.notify_one();
}

void ThreadPool::ThreadFunc()
{
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream sin;
    sin << tid;
    std::cout << "worker thread is running :" << tid << std::endl;
    Task task;
    while (started_)
    {
        task = NULL;
        {
            // 无名作用域
            std::unique_lock<std::mutex> lock(mutex_); //unique_lock支持解锁又上锁情况
            while (taskqueue_.empty() && started_)
            {
                condition_.wait(lock);
            }
            if (!started_)
            {
                break;
            }
            //std::cout << "wake up" << tid << std::endl;
            //std::cout << "size :" << taskqueue_.size() << std::endl;
            task = taskqueue_.front();
            taskqueue_.pop();
        }
        if (task)
        {
            try
            {
                //std::cout << "ThreadPool::ThreadFunc" << std::endl;
                task();
            }
            catch (std::bad_alloc &ba)
            {
                std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task: " << ba.what() << '\n';
                while (1)
                    ;
            }
            //task();//task中的IO过程可以使用协程优化，让出CPU
        }
    }
}