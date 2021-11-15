
#include "EventLoopThread.h"
#include <iostream>
#include <sstream>
//#include <queue>
//#include <mutex>  
//#include <condition_variable>

EventLoopThread::EventLoopThread()
    : th_(),
    threadid_(-1),
    threadname_("IO thread "),
    loop_(NULL)
{
    
}

EventLoopThread::~EventLoopThread()
{
    //线程结束时清理
    std::cout << "Clean up the EventLoopThread id: " << std::this_thread::get_id() << std::endl;
    loop_->Quit();//停止IO线程运行
    th_.join();//清理IO线程，防止内存泄漏，因为pthread_created回calloc
}

EventLoop* EventLoopThread::GetLoop()
{
    return loop_;
}
void EventLoopThread::Start()
{
    //create thread
    th_ = std::thread(&EventLoopThread::ThreadFunc, this);    
}

void EventLoopThread::ThreadFunc()
{
    EventLoop loop;
    loop_ = &loop;

    threadid_ = std::this_thread::get_id();
    std::stringstream sin;
    sin << threadid_;    
    threadname_ += sin.str();

    std::cout << "in the thread:" << threadname_ << std::endl;   
    try
    {
        //std::cout << "EventLoopThread::ThreadFunc " << std::endl;
        loop_->loop();
    }
    catch (std::bad_alloc& ba)
    {
        std::cerr << "bad_alloc caught in EventLoopThread::ThreadFunc loop: " << ba.what() << '\n';
    }
    //loop_->loop();
}
