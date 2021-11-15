
#include "ThreadPool.h"
#include <deque>
#include <unistd.h>

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
    for(int i = 0; i < threadnum_; ++i)
    {
        threadlist_[i]->join();
    }
    for(int i = 0; i < threadnum_; ++i)
    {
        delete threadlist_[i];
    }
    threadlist_.clear();
}

void ThreadPool::Start()
{
    if(threadnum_ > 0)
    {
        started_ = true;
        for(int i = 0; i < threadnum_; ++i)
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
    while(started_)
    {
        task = NULL;
        {
            // 无名作用域
            std::unique_lock<std::mutex> lock(mutex_);//unique_lock支持解锁又上锁情况
            while(taskqueue_.empty() && started_)
            {
                condition_.wait(lock);
            }
            if(!started_)
            {
                break;
            }
            //std::cout << "wake up" << tid << std::endl;
            //std::cout << "size :" << taskqueue_.size() << std::endl;
            task = taskqueue_.front();    
            taskqueue_.pop();            
        }
        if(task)
        {
            try
            {
                //std::cout << "ThreadPool::ThreadFunc" << std::endl;
                task();
            }
            catch (std::bad_alloc& ba)
            {
                std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task: " << ba.what() << '\n';
                while(1);
            }
            //task();//task中的IO过程可以使用协程优化，让出CPU
        }                        
    }
}


// 测试
// class Test
// {
// private:
//     /* data */
// public:
//     Test(/* args */);
//     ~Test();
//     void func()
//     {
//         std::cout << "Work in thread: " << std::this_thread::get_id() << std::endl;
//     }
// };

// Test::Test(/* args */)
// {
// }

// Test::~Test()
// {
// }


// int main()
// {
//     Test t;
//     ThreadPool tp(2);
//     tp.Start();
//     for (int i = 0; i < 1000; ++i)
//     {
//         std::cout << "addtask" << std::this_thread::get_id() << std::endl;
//         tp.AddTask(std::bind(&Test::func, &t));
//     }
// }