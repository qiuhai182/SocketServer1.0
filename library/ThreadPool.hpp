
// 线程池类

// 使用的同步原语有
// pthread_mutex_t mutex_l;     //互斥锁
// pthread_cond_t condtion_l;   //条件变量

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

class ThreadPool
{
public:
    typedef std::function<void()> Task;
    ThreadPool(int threadnum = 0);
    ~ThreadPool();
    void Start();           // 标志为运行状态，创建threadNum_个子线程作为工作线程并启动线程
    void Stop();            // 标志为停止运行状态，唤醒所有线程，线程分离异步运行，清空线程池
    void AddTask(Task task);// 添加一个任务到任务列表taskQueue_，随机唤醒一个工作线程执行一个任务
    void ThreadFunc();      // 线程回调函数，单次遍历，加锁取出taskQueue_的一个任务并执行
    int GetThreadNum();     // 获取工作线程数量

private:
    bool started_;      // 线程池运行状态
    int threadNum_;     // 线程池控制工作线程数量
    std::mutex mutex_;
    std::condition_variable condition_;
    std::vector<std::thread *> threadList_; // 工作线程列表
    std::queue<Task> taskQueue_;    // 任务队列，由线程池及其子工作线程间共享，线程池负责添加，工作线程执行
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
 * 标志为运行状态，创建threadNum_个子线程作为工作线程
 * 堆内创建线程，线程创建直接运行
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
 * 标志为停止运行状态，唤醒所有线程，线程分离异步运行，清空线程池
 * 
 */
void ThreadPool::Stop()
{
    started_ = false;
    condition_.notify_all();
    for (auto i : threadList_)
    {
        // 线程分离，异步线程
        std::cout << "启动并分离线程：" << i << std::endl;
        i->detach();
        // i->join();
    }
    threadList_.clear();
}

/*
 * 添加一个任务到任务列表taskQueue_
 * 随机唤醒一个工作线程执行一个任务
 * 
 */
void ThreadPool::AddTask(Task task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push(task);
    }
    condition_.notify_one();
}

/*
 * 线程回调函数，在每个工作线程内运行的回调函数
 * 单次遍历，加锁取出taskQueue_的一个任务并执行
 * 
 */
void ThreadPool::ThreadFunc()
{
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream sin;
    sin << tid;
    std::cout << "工作线程: " << tid << "启动" << std::endl;
    Task task;
    while (started_)
    {
        task = NULL;
        {
            // 无名作用域
            std::unique_lock<std::mutex> lock(mutex_);
            while (taskQueue_.empty() && started_)
            {
                // 线程阻塞并等待被唤醒
                condition_.wait(lock);
            }
            // 线程被唤醒发现需要退出工作状态，退出循环
            if (!started_)
            {
                break;
            }
            std::cout << "工作线程: " << tid << "已唤醒，现有待处理任务数量: " << taskQueue_.size() << std::endl;
            // 取出队头待处理任务
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
                std::cerr << "bad_alloc错误捕获于函数ThreadPool::ThreadFunc，报错: " << ba.what() << '\n';
                while (1);
            }
        }
    }
    if (!started_)
    {
        std::cout << "工作子线程：" << tid << "结束" << std::endl;
    }
}

/*
 * 获取工作线程数量
 * 
 */
int ThreadPool::GetThreadNum()
{
    return threadNum_;
}
