
// Poller类，对epoll的封装

#pragma once

#include <iostream>
#include <vector>
#include <mutex>
#include <map>
#include <sys/epoll.h>
#include <errno.h>
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
    int pollFd_;
    std::map<int, Channel *> channelMap_;
    std::mutex mutex_;
    void poll(ChannelList &activeChannelList);
    void AddChannel(Channel *pchannel);    // 添加事件，EPOLL_CTL_ADD
    void RemoveChannel(Channel *pchannel); // 移除事件，EPOLL_CTL_DEL
    void UpdateChannel(Channel *pchannel); // 修改事件，EPOLL_CTL_MOD
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

void Poller::poll(ChannelList &activeChannelList)
{
    int timeout = TIMEOUT;
    // 监听一批次epoll网络请求
    int nfds = epoll_wait(pollFd_, &*eventList_.begin(), (int)eventList_.capacity(), timeout);
    if (nfds == -1)
    {
        perror("epoll wait error");
    }
    for (int i = 0; i < nfds; ++i)
    {
        // 遍历获取每个网络请求事件
        int events = eventList_[i].events;
        // int fd = eventList_[i].data.fd;
        Channel *pchannel = (Channel *)eventList_[i].data.ptr;
        int fd = pchannel->GetFd();
        std::map<int, Channel *>::const_iterator iter;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            iter = channelMap_.find(fd);
        }
        if (iter != channelMap_.end())
        {
            pchannel->SetEvents(events);
            activeChannelList.push_back(pchannel);
        }
        else
        {
            std::cout << "not find channel!" << std::endl;
        }
    }
    if (nfds == (int)eventList_.capacity())
    {
        // 扩大eventList_预分配容量
        std::cout << "resize:" << nfds << std::endl;
        eventList_.resize(nfds * 2);
    }
    // eventList_.clear();
}

void Poller::AddChannel(Channel *pchannel)
{
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

void Poller::RemoveChannel(Channel *pchannel)
{
    int fd = pchannel->GetFd();
    struct epoll_event ev;
    ev.events = pchannel->GetEvents();
    // ev.data.fd = fd
    ev.data.ptr = pchannel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelMap_.erase(fd);
    }
    if (epoll_ctl(pollFd_, EPOLL_CTL_DEL, fd, &ev) == -1)
    {
        perror("epoll del error");
        exit(1);
    }
}

void Poller::UpdateChannel(Channel *pchannel)
{
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
