
// Poller类，对epoll的封装

#pragma once

#include <iostream>
#include <vector>
#include <mutex>
#include <map>
#include <sys/epoll.h>
#include "Channel.hpp"

#define MAXEVENTNUM 4096 // 最大触发事件数量
#define TIMEOUT 1000     // epoll_wait 超时时间设置

class Poller
{
public:
    typedef std::vector<Channel *> ChannelList;
    std::vector<struct epoll_event> eventList_;
    Poller();
    ~Poller();
    std::mutex mutex_;
    int pollFd_;                               // epoll监听描述符
    std::map<int, Channel *> channelMap_;      // 套接字描述符->Channel实例，存储所有连接Channel实例
    void AddChannel(Channel *pchannel);        // 添加事件，EPOLL_CTL_ADD
    void RemoveChannel(Channel *pchannel);     // 移除事件，EPOLL_CTL_DEL
    void UpdateChannel(Channel *pchannel);     // 修改事件，EPOLL_CTL_MOD
    void poll(ChannelList &activeChannelList); // 获取一批次新事件
};

Poller::Poller()
    : pollFd_(-1),
      eventList_(MAXEVENTNUM),
      channelMap_(),
      mutex_()
{
    pollFd_ = epoll_create(256); // 最大监听256个连接
    if (pollFd_ == -1)
    {
        perror("epoll_create error");
        exit(1);
    }
}

Poller::~Poller()
{
    close(pollFd_);
}

/*
 * 向pollFd_添加监听Channel对应的新连接
 *
 */
void Poller::AddChannel(Channel *pchannel)
{
    std::cout << "输出测试：Poller::AddChannel " << std::endl;
    struct epoll_event ev;
    ev.events = pchannel->GetEvents();
    ev.data.ptr = pchannel;
    int fd = pchannel->GetFd();
    // ev.data.fd = fd; // data是联合体
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelMap_[fd] = pchannel;
    }
    if (epoll_ctl(pollFd_, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll add error");
        exit(1);
    }
}

/*
 * 移除pollFd_里Channel对应的连接的监听
 *
 */
void Poller::RemoveChannel(Channel *pchannel)
{
    std::cout << "输出测试：Poller::RemoveChannel " << std::endl;
    int fd = pchannel->GetFd();
    struct epoll_event ev;
    ev.events = pchannel->GetEvents();
    // ev.data.fd = fd
    ev.data.ptr = pchannel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelMap_.erase(fd);
    }
    // epoll添加客户端连接监听
    if (epoll_ctl(pollFd_, EPOLL_CTL_DEL, fd, &ev) == -1)
    // if (epoll_ctl(pollFd_, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        perror("epoll del error");
        exit(1);
    }
}

/*
 * 更新pollFd_里Channel对应监听事件的信息
 *
 */
void Poller::UpdateChannel(Channel *pchannel)
{
    std::cout << "输出测试：Poller::UpdateChannel " << std::endl;
    int fd = pchannel->GetFd();
    struct epoll_event ev;
    ev.events = pchannel->GetEvents();
    // ev.data.fd = fd;
    ev.data.ptr = pchannel;
    if (epoll_ctl(pollFd_, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        perror("epoll update error");
        exit(1);
    }
}

/*
 * 封装epoll_wait函数，获取一批次新任务
 * 由事件池获取新任务时调用
 *
 */
void Poller::poll(ChannelList &activeChannelList)
{
    int timeout = TIMEOUT;
    // 监听一批次epoll网络请求
    int nfds = epoll_wait(pollFd_, &*eventList_.begin(), (int)eventList_.capacity(), timeout);
    if (nfds == -1)
    {
        perror("epoll wait error");
    }
    if (nfds)
        std::cout << std::endl
                  << "输出测试：Poller::poll Poller监听到" << nfds << "个已连接客户端的事件待处理" << std::endl;
    // 遍历获取每个网络请求事件
    for (int i = 0; i < nfds; ++i)
    {
        int events = eventList_[i].events;
        Channel *pchannel = (Channel *)eventList_[i].data.ptr;
        int fd = pchannel->GetFd();
        std::map<int, Channel *>::const_iterator iter;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            iter = channelMap_.find(fd);
        }
        // 设置连接Channel实例新连接事件
        if (iter != channelMap_.end())
        {
            std::cout << "输出测试：Poller::poll Poller已找到一个已连接事件的Channel实例，连接socketfd：" << fd << std::endl;
            pchannel->SetEvents(events);
            activeChannelList.push_back(pchannel);
        }
        else
        {
            std::cout << "输出测试：Poller::poll Poller未找到该连接的Channel实例，连接socketfd：" << fd << std::endl;
        }
    }
    // epoll事件列表满，翻倍扩大eventList_预分配容量
    if (nfds == (int)eventList_.capacity())
    {
        std::cout << "Poller::poll 输出测试：epoll池容量满，容量翻倍" << nfds << std::endl;
        eventList_.resize(nfds * 2);
    }
    eventList_.clear();
}
