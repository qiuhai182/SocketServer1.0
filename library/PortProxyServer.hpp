#pragma once

#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <netinet/tcp.h>

#define PROXY_PORT 6000 // 端口转发代理服务器工作端口，一切外部请求都通过此端口访问内部服务

typedef struct _SockPair_
{
	int m_cli_socket;
	int m_svr_socket;
	_SockPair_()
	{
		m_cli_socket = -1;
		m_svr_socket = -1;
	}
} SockPair;

typedef struct sockaddr *SockAddr;

class PortProxyServer
{
	;
};

int init_listen_socket();
int wait_client(int listen_socket, const char *server_ip, unsigned int server_port);
int connect_server(const char *serverip, unsigned int serverport);
void *connect_thread(void *p);
void *RecandSendData(int ClientSocket, int ServerSocket);
int main(int argc, const char **argv);


int init_listen_socket()
{
	int listenFD;
	if (-1 == (listenFD = socket(AF_INET, SOCK_STREAM, 0)))
	{
		printf("socket() failed with error\n");
		return -1;
	}
	sockaddr_in InternetAddr;
	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	InternetAddr.sin_port = htons(PROXY_PORT);
	int reuseopt = 1;
	setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseopt, sizeof(&reuseopt));
	if (-1 == bind(listenFD, (SockAddr)&InternetAddr, sizeof(InternetAddr)))
	{
		printf("代理服务器绑定本地端口<%d>失败\n", PROXY_PORT);
		return -1;
	}
	if (listen(listenFD, 5))
	{
		printf("代理服务器监听本地端口<%d>失败\n", PROXY_PORT);
		return -1;
	}
	printf("代理服务器成功绑定并监听本地端口：%u\n", PROXY_PORT);
	return listenFD;
}

int wait_client(int listen_socket, const char *server_ip, unsigned int server_port)
{
	struct pollfd pollFd;
	pollFd.fd = listen_socket;
	pollFd.events = POLLRDNORM;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	SockPair *pSockPair = NULL;
	for (;;)
	{
		while (0 == poll(&pollFd, 1, 1000))
			continue;
		if (pollFd.revents & POLLRDNORM)
		{
			pSockPair = new SockPair;
			pSockPair->m_cli_socket = accept(listen_socket, (struct sockaddr *)&cliaddr, &clilen);
			pSockPair->m_svr_socket = connect_server(server_ip, server_port);
			if (pSockPair->m_svr_socket == -1)
			{
				delete pSockPair;
				printf("connect server failed\n");
				continue;
			}
			// 开启新线程，处理转发
			pthread_t pid;
			int ret = pthread_create(&pid, NULL, connect_thread, pSockPair);
			if (ret != -1)
				pthread_detach(pid);
			else
				printf("create thread failed!\n");
		}
	}
	return 0;
}

int connect_server(const char *serverip, unsigned int serverport)
{
	if (serverport != 0)
	{
		printf("begin connect machine:%s:%u\n", serverip, serverport);
		int s = socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in addrSrv;
		addrSrv.sin_addr.s_addr = inet_addr(serverip);
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(serverport);
		int re = connect(s, (sockaddr *)&addrSrv, sizeof(sockaddr));
		if (-1 != re)
		{
			printf("连接服务器[%s:%u]成功\n", serverip, serverport);
			return s;
		}
		else
		{
			printf("connect machine: %s:%u fail\n", serverip, serverport);
			close(s);
		}
	}
	return -1;
}

void *connect_thread(void *p)
{
	SockPair *pSockPair = (SockPair *)p;
	RecandSendData(pSockPair->m_cli_socket, pSockPair->m_svr_socket);
	delete pSockPair;
	return NULL;
}

void *RecandSendData(int ClientSocket, int ServerSocket)
{
	struct pollfd pollFd[2];
	pollFd[0].fd = ClientSocket;
	pollFd[0].events = POLLRDNORM;
	pollFd[1].fd = ServerSocket;
	pollFd[1].events = POLLRDNORM;
	int buflen = 3 * 1024 * 1024;
	char *buf = new char[buflen];
	// TCP状态检测，主要是检测断开连接
	struct tcp_info tcpinfo;
	int tcpinfolen = sizeof(tcpinfo);
	for (;;)
	{
		while (0 == poll(pollFd, 2, 1000))
			continue;
		int index = 0;
		for (; index < 2; index++)
		{
			if (pollFd[index].revents & POLLRDNORM)
			{
				break;
			}
		}
		if (index >= 2)
			continue;
		int this_fd = pollFd[index].fd;
		int other_fd = pollFd[(index ? 0 : 1)].fd;
		// 从this_fd接收数据
		int rcv_len = recv(this_fd, buf, buflen, 0);
		if (rcv_len >= 0)
		{
			// printf("socket[%d] = %d\t接收到数据(%dB)：%s\n", index, this_fd, rcv_len, buf);
		}
		else if (rcv_len == -1)
		{
			printf("接收数据失败[%d]！\n", index);
			break;
		}
		// 转发数据给other_fd
		int snd_len = send(other_fd, buf, rcv_len, 0);
		if (snd_len >= 0)
		{
			// printf("socket[%d] = %d\t发送数据(%dB)：%s\n", index, other_fd, snd_len, buf);
		}
		else if (snd_len == -1)
		{
			printf("发送失败[%d]！\n", index);
			break;
		}

		// TCP连接状态检查
		getsockopt(this_fd, IPPROTO_TCP, TCP_INFO, (void *)&tcpinfo, (socklen_t *)&tcpinfolen);
		if (tcpinfo.tcpi_state == TCP_CLOSE_WAIT)
		{
			break;
		}
	} /// end of while true
	delete[] buf;
}

int main(int argc, const char **argv)
{
	const char *server_ip = "192.168.137.1"; // 待转发服务ip
	unsigned int server_port = 8888;		 // 待转发服务端口
	if (argc >= 3)
	{
		server_ip = argv[1];
		server_port = (unsigned int)atoi(argv[2]);
	}
	int listen_socket = init_listen_socket();
	if (listen_socket == -1)
	{
		printf("listen port failed\n");
		return -1;
	}
	wait_client(listen_socket, server_ip, server_port);

	return 0;
}
