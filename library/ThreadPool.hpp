
// 线程池类

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
    int GetThreadNum();

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
 * 启动线程池，新开threadNum_个子线程作为工作线程
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
 * 线程池停止执行，唤醒并关闭所有工作线程
 * 
 */
void ThreadPool::Stop()
{
    started_ = false;
    condition_.notify_all();
    for (auto i : threadList_)
    {
        i->detach();
    }
    threadList_.clear();
}

/*
 * 为线程池任务队列taskQueue_加锁添加任务
 * 随机唤醒一个工作线程
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
 * 线程回调函数，在每个工作线程内长期执行直至子线程被回收
 * 单次遍历，加锁取出taskQueue_的一个任务并执行
 */
void ThreadPool::ThreadFunc()
{
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream sin;
    sin << tid;
    std::cout << "工作子线程: " << tid << std::endl;
    Task task;
    while (started_)
    {
        task = NULL;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (taskQueue_.empty() && started_)
            {
                // 线程阻塞并等待被唤醒
                condition_.wait(lock);
            }
            if (!started_)
            {
                break;
            }
            std::cout << "工作线程唤醒: " << tid << std::endl;
            std::cout << "现有待处理任务数量: " << taskQueue_.size() << std::endl;
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
        std::cout << "工作子线程退出：" << tid << std::endl;
    }
}

/*
 * 获取线程数量
 * 
 */
int ThreadPool::GetThreadNum()
{
    return threadNum_;
}
