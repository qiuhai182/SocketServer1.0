
// 服务器socket类，封装socket描述符及相关的初始化操作

#pragma once

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Socket
{
private:
    // 服务器socket文件描述符
    int _socketFd;
    
public:
    Socket(/* args */);
    ~Socket();
    // 获取fd
    int fd() const { return _socketFd; }   
    // 设置地址重用
    void SetReuseAddr();
    // 设置非阻塞
    void SetNonblocking();
    // 绑定地址
    bool BindAddress(int serverport);
    // 开启监听
    bool Listen();
    // accept获取连接
    int Accept(struct sockaddr_in &clientaddr);
    // 关闭服务器fd
    bool Close();

};

Socket::Socket()
{
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == _socketFd)
    {
        perror("socket create fail!");
        exit(-1);
    }
}

Socket::~Socket()
{
    close(_socketFd);
}

void Socket::SetReuseAddr()
{
    int on = 1;
    setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

void Socket::SetNonblocking()
{
    // 获取属性
    int opts = fcntl(_socketFd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(_socketFd,GETFL)");
        exit(1);
    }
    // 或操作设置非阻塞
    if (fcntl(_socketFd, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        perror("fcntl(_socketFd,SETFL,opts)");
        exit(1);
    }
}

bool Socket::BindAddress(int serverPort)
{
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);
    int resval = bind(_socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (resval == -1)
    {
        close(_socketFd);
        perror("error bind");
        exit(1);
    }
    return true;
}

bool Socket::Listen()
{
    if (listen(_socketFd, 8192) < 0)
    {
        perror("error listen");
        close(_socketFd);
        exit(1);
    }
    return true;
}

int Socket::Accept(struct sockaddr_in &clientAddr)
{
    socklen_t lengthOfClientAddr = sizeof(clientAddr);
    int clientFd = accept(_socketFd, (struct sockaddr *)&clientAddr, &lengthOfClientAddr);
    if (clientFd < 0 && errno == EAGAIN)
    {
        return 0;
    }
    return clientFd;
}

bool Socket::Close()
{
    close(_socketFd);
    return true;
}
