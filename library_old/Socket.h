
// 服务器socket类，封装socket描述符及相关的初始化操作

#ifndef _SOCKET_H_
#define _SOCKET_H_

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


#endif