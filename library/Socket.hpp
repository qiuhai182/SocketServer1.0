
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

/*
 * 设置地址重用，用于服务重启
 * 
 */
void Socket::SetReuseAddr()
{
    int on = 1;
    setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

/*
 * 设置非阻塞IO
 * 
 */
void Socket::SetNonblocking()
{
    int opts = fcntl(_socketFd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(_socketFd,GETFL)");
        exit(1);
    }
    if (fcntl(_socketFd, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        perror("fcntl(_socketFd,SETFL,opts)");
        exit(1);
    }
}

/*
 * 套接字绑定IP和端口
 * 
 */
bool Socket::BindAddress(int serverPort)
{
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);
    // 套接字绑定IP和端口
    int resval = bind(_socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (resval == -1)
    {
        close(_socketFd);
        perror("error bind");
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
    // 开始监听
    if (listen(_socketFd, 8192) < 0)
    {
        perror("error listen");
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
    // 响应一个连接请求
    socklen_t lengthOfClientAddr = sizeof(clientAddr);
    int clientFd = accept(_socketFd, (struct sockaddr *)&clientAddr, &lengthOfClientAddr);
    if (clientFd < 0 && errno == EAGAIN)
    {
        return 0;
    }
    std::cout << "输出测试：服务Socket接受一个连接，sockfd：" << clientFd << std::endl;
    return clientFd;
}

/*
 * 关闭套接字连接
 * 
 */
bool Socket::Close()
{
    close(_socketFd);
    return true;
}
