
// socket类：
//  封装socket描述符及相关函数

#pragma once

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "LogServer.hpp"

class Socket
{
private:
    int _socketFd;

public:
    Socket();
    ~Socket();
    int fd() const { return _socketFd; } // 获取套接字描述符
    void SetReuseAddr();    // 地址重用
    void SetNonblocking();  // 非阻塞IO
    bool BindAddress(int serverport);   // 绑定监听IP:端口
    bool Listen();  // 监听启动
    int Accept(struct sockaddr_in &clientaddr); // 响应连接
    bool Close(); // 关闭套接字连接

};

Socket::Socket()
{
    LOG(LoggerLevel::INFO, "%s，sockfd：%d\n", "函数触发", _socketFd);
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == _socketFd)
    {
        LOG(LoggerLevel::INFO, "创建socket实例失败，sockfd：%d\n", _socketFd);
        std::cout << "Socket::Socket 创建socket实例失败，退出" << std::endl;
        perror("创建socket实例失败");
        exit(-1);
    }
}

Socket::~Socket()
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd: %d\n", _socketFd);
    close(_socketFd);
}

/*
 * 设置地址重用，用于服务重启
 * 
 */
void Socket::SetReuseAddr()
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd: %d\n", _socketFd);
    int on = 1;
    setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

/*
 * 设置非阻塞IO
 * 
 */
void Socket::SetNonblocking()
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd: %d\n", _socketFd);
    int opts = fcntl(_socketFd, F_GETFL);
    if (opts < 0)
    {
        LOG(LoggerLevel::ERROR, "获取socket的描述符失败，sockfd: %d\n", _socketFd);
        std::cout << "Socket::SetNonblocking 获取socket的描述符失败，退出" << std::endl;
        perror("获取socket的描述符失败（fcntl(_socketFd, GETFL)）");
        exit(1);
    }
    if (fcntl(_socketFd, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        LOG(LoggerLevel::ERROR, "更改socket的描述符失败，sockfd: %d\n", _socketFd);
        std::cout << "Socket::SetNonblocking 更改socket的描述符失败，退出" << std::endl;
        perror("改socket的描述符失败（fcntl(_socketFd,SETFL,opts)）");
        exit(1);
    }
}

/*
 * 套接字绑定IP和端口
 * 
 */
bool Socket::BindAddress(int serverPort)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd: %d\n", _socketFd);
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    // serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);
    // 套接字绑定IP和端口
    int resval = bind(_socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (resval == -1)
    {
        LOG(LoggerLevel::ERROR, "为socket绑定ip失败，sockfd: %d\n", _socketFd);
        std::cout << "Socket::BindAddress 为socket绑定ip失败，退出" << std::endl;
        close(_socketFd);
        perror("为socket绑定ip失败");
        exit(1);
    }
    return true;
}

/*
 * 套接字监听IP和端口
 * 
 */
bool Socket::Listen()
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd: %d\n", _socketFd);
    // 开始监听
    if (listen(_socketFd, 8192) < 0)
    {
        LOG(LoggerLevel::ERROR, "启动socket监听失败，sockfd: %d\n", _socketFd);
        std::cout << "Socket::Listen 启动socket监听失败，退出" << std::endl;
        perror("启动socket监听失败");
        close(_socketFd);
        exit(1);
    }
    return true;
}

/*
 * 套接字响应IP连接
 * 
 */
int Socket::Accept(struct sockaddr_in &clientAddr)
{
    LOG(LoggerLevel::INFO, "函数触发，sockfd: %d\n", _socketFd);
    // 响应一个连接请求
    socklen_t lengthOfClientAddr = sizeof(clientAddr);
    int clientFd = accept(_socketFd, (struct sockaddr *)&clientAddr, &lengthOfClientAddr);
    if (clientFd < 0 && errno == EAGAIN)
    {
        return 0;
    }
    LOG(LoggerLevel::INFO, "服务Socket接受一个连接，服务sockfd: %d，客户连接sockfd：%d\n", _socketFd, clientFd);
    std::cout << "Socket::Accept 服务Socket接受一个连接，sockfd：" << clientFd << std::endl;
    return clientFd;
}

/*
 * 关闭套接字连接
 * 
 */
bool Socket::Close()
{
    LOG(LoggerLevel::ERROR, "函数触发，sockfd: %d\n", _socketFd);
    close(_socketFd);
    return true;
}
