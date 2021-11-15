
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
    typedef std::function<void()> Task;
    ThreadPool(int threadnum = 0);
    ~ThreadPool();
    void Start();
    void Stop();
    void AddTask(Task task);
    void ThreadFunc();
    int GetThreadNum()
    {
        return threadNum_;
    }

private:
    bool started_;
    int threadNum_;
    std::vector<std::thread *> threadList_;
    std::queue<Task> taskQueue_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

ThreadPool::ThreadPool(int threadnum)
    : started_(false),
      threadNum_(threadnum),
      threadList_(),
      taskQueue_(),
      mutex_(),
      condition_()
{
}

ThreadPool::~ThreadPool()
{
    std::cout << "析构线程池" << std::endl;
    Stop();
    for (int i = 0; i < threadNum_; ++i)
    {
        threadList_[i]->join();
    }
    for (int i = 0; i < threadNum_; ++i)
    {
        delete threadList_[i];
    }
    threadList_.clear();
}

/*
 * 
 * 
 */
void ThreadPool::Start()
{
    if (threadNum_ > 0)
    {
        started_ = true;
        for (int i = 0; i < threadNum_; ++i)
        {
            std::thread *pthread = new std::thread(&ThreadPool::ThreadFunc, this);
            threadList_.push_back(pthread);
        }
    }
}

/*
 * 
 * 
 */
void ThreadPool::Stop()
{
    started_ = false;
    condition_.notify_all();
}

/*
 * 
 * 
 */
void ThreadPool::AddTask(Task task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push(task);
    }
    // 随机解锁正在等待当前条件的线程中的一个
    condition_.notify_one();
}

/*
 * 
 * 
 */
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
            std::unique_lock<std::mutex> lock(mutex_);
            while (taskQueue_.empty() && started_)
            {
                condition_.wait(lock);
            }
            if (!started_)
            {
                break;
            }
            std::cout << "wake up" << tid << std::endl;
            std::cout << "size :" << taskQueue_.size() << std::endl;
            task = taskQueue_.front();
            taskQueue_.pop();
        }
        if (task)
        {
            try
            {
                task();
            }
            catch (std::bad_alloc &ba)
            {
                std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task: " << ba.what() << '\n';
                while (1);
            }
        }
    }
}
