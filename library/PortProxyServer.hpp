
// PortProxy端口转发服务类
//	管理使用一个线程池，在客户请求并发量达到一定规模后，会出现高延迟问题
//	端口转发服务若与其他服务共同部署于一台服务器上，会占用系统资源，并发量越高，占用资源可能成倍增加
//	总之，端口转发服务多了一层中转，要想实现高并发是一件颇具难度的事情，本文只是一种简单实现

#pragma once

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <sys/poll.h>
#include <netinet/tcp.h>
#include "Timer.hpp"
#include "Resource.hpp"
#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "ThreadPool.hpp"
#include "TypeIdentify.hpp"
#include "TcpConnection.hpp"

class PortProxyServer
{
public:
	PortProxyServer(const int workThreadNum = 2, const int loopThreadNum = 0,
				const std::string &serv_ip = "0.0.0.0", const unsigned int serv_port = 8000);
    ~PortProxyServer();
	// 存入键值对targetServerName:vector<pair<target_ip, target_port>>、targetServerName:proxyIndex
	void registerPortProxy(const std::string &targetServerName, const std::string &target_ip, unsigned int target_port);
	void Start();
	
private:
    std::string serviceName_;
	std::map<std::string, std::vector<std::pair<std::string, int>>> targetServices;	// 存储目标服务可用IP:PORT地址
	// 存储目标服务当前可用IP：PORT，若某服务在targetServices内有不止1个可跳转地址则对应targetSelectIndex负责循环依次调用
	std::map<std::string, int> targetSelectIndex;
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
	const std::string tcpServerIP_;		// 端口代理服务的本地绑定ip
	const int tcpServerPort_; 			// 端口代理服务监听端口
	EventLoop loop;						// 内置一个EventLoop作为主逻辑IO处理对象
    TcpServer *tcpserver_;  			// 基础网络服务TcpServer
    ThreadPool *threadpool_;            // 线程池
    int  getFileSize(char *file_name);  // 获取文件大小	
	void HttpError(spTcpConnection &sptcpconn, const std::string &short_msg);
	void ProxyProcess(spTcpConnection &sptcpconn); // 处理请求并响应，代理两个连接之间进行数据通信函数
	void HandleMessage(spTcpConnection &sptcpconn);
	void HandleError(spTcpConnection &sptcpconn);
	void HandleClose(spTcpConnection &sptcpconn);
	void HandleSendComplete(spTcpConnection &sptcpconn);
	
};

PortProxyServer::PortProxyServer(const int workThreadNum, const int loopThreadNum,
								 const std::string &serv_ip, const unsigned int serv_port)
    : serviceName_("PortProxyService"),
	  tcpServerIP_(serv_ip),
	  tcpServerPort_(serv_port),
      threadpool_(new ThreadPool(workThreadNum)),
      tcpserver_(new TcpServer(&loop, tcpServerPort_, loopThreadNum))
{
    // 基于TcpServer设置PortProxyServer服务函数，在TcpServer内触发调用PortProxyServer的成员函数，类似于信号槽机制
    // 采用覆盖注册函数模式，RegisterHandler的coverAllService参数置为true，使tcpserver_只提供端口代理服务
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
	tcpserver_->RegisterHandler(TcpServer::CoverServiceName, TcpServer::ReadMessageHandler, std::bind(&PortProxyServer::HandleMessage, this, std::placeholders::_1), true);
    tcpserver_->RegisterHandler(TcpServer::CoverServiceName, TcpServer::SendOverHandler, std::bind(&PortProxyServer::HandleSendComplete, this, std::placeholders::_1), true);
    tcpserver_->RegisterHandler(TcpServer::CoverServiceName, TcpServer::CloseConnHandler, std::bind(&PortProxyServer::HandleClose, this, std::placeholders::_1), true);
    tcpserver_->RegisterHandler(TcpServer::CoverServiceName, TcpServer::ErrorConnHandler, std::bind(&PortProxyServer::HandleError, this, std::placeholders::_1), true);
    tcpserver_->RegisterHandler(TcpServer::CoverServiceName, TcpServer::CoverHandler, std::bind(&PortProxyServer::ProxyProcess, this, std::placeholders::_1), true);
}

PortProxyServer::~PortProxyServer()
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
    delete threadpool_;
	delete tcpserver_;
}

void PortProxyServer::Start()
{    
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
	try
    {    
		if(tcpServerPort_)
        	threadpool_->Start();
        loop.loop();
    }
    catch (std::bad_alloc &ba)
    {
		LOG(LoggerLevel::ERROR, "捕获错误：bad_alloc，描述：%s，工作端口：%s:%d\n", ba.what(), tcpServerIP_.data(), tcpServerPort_);
        std::cerr << "PortProxyServer::Start 捕获错误：bad_alloc，描述：" << ba.what() << '\n';
    }
}

/*
 * PortProxyServer模式处理收到的请求
 *
 */
void PortProxyServer::HandleMessage(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
    // 修改定时器参数
    sptcpconn->GetTimer()->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    sptcpconn->SetAsyncProcessing(false);
    if (false == sptcpconn->GetReqHealthy())
    {
		HttpError(sptcpconn, "目标服务请求失败，请稍后重试");
        return;
    }
    if (threadpool_->GetThreadNum() > 0)
    {
        // 已开启线程池，设置异步处理标志
        sptcpconn->SetAsyncProcessing(true);
        // 线程池在此添加任务并唤醒一工作线程执行之
        threadpool_->AddTask([&]()
                             {
                                // 执行动态绑定的处理函数
    							LOG(LoggerLevel::INFO, "工作线程执行sptcpconn的绑定函数，此时sptcpconn->IsDisconnected()：%s，sockfd：%d工作端口：%s:%d\n", sptcpconn->IsDisconnected() ? "true" : "false", sptcpconn->fd(), tcpServerIP_.data(), tcpServerPort_);
                                if(sptcpconn->IsDisconnected())
                                {
    								LOG(LoggerLevel::ERROR, "工作线程执行sptcpconn的绑定函数，此时sptcpconn已关闭，不作处理，sockfd：%d工作端口：%s:%d\n", sptcpconn->fd(), tcpServerIP_.data(), tcpServerPort_);
                                }
                                else
                                {
                                    try
                                    {
                                        sptcpconn->GetReqHandler()(sptcpconn);
                                    }
                                    catch(std::bad_function_call)
                                    {
    									LOG(LoggerLevel::ERROR, "工作线程执行sptcpconn的绑定函数报错：std::bad_function_call，连接绑定函数异常，sockfd：%d工作端口：%s:%d\n", sptcpconn->fd(), tcpServerIP_.data(), tcpServerPort_);
                                        std::cout << "PortProxyServer::HandleMessage 工作线程执行sptcpconn的绑定函数报错：std::bad_function_call，连接绑定函数异常，sockfd：" << sptcpconn->fd() << std::endl;
                                        sptcpconn->SetAsyncProcessing(false);
                                    }
                                    sptcpconn->SetAsyncProcessing(false);
                                } });
    }
    else
    {
        // 没有开启线程池，执行动态绑定的处理函数
        try
        {
            sptcpconn->GetReqHandler()(sptcpconn);
        }
        catch (std::bad_function_call)
        {
    		LOG(LoggerLevel::ERROR, "执行sptcpconn的绑定函数报错：std::bad_function_call，连接绑定函数异常，sockfd：%d工作端口：%s:%d\n", sptcpconn->fd(), tcpServerIP_.data(), tcpServerPort_);
            std::cout << "PortProxyServer::HandleMessage 执行sptcpconn的绑定函数报错：std::bad_function_call，连接绑定函数异常，sockfd：" << sptcpconn->fd() << std::endl;
        }
    }
}

void PortProxyServer::registerPortProxy(const std::string &targetServerName, const std::string &target_ip, unsigned int target_port)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
	if(targetServices.end() == targetServices.find(targetServerName))
	{
		std::vector<std::pair<std::string, int>> newServices;
		targetServices[targetServerName] = std::move(newServices);
	}
	std::pair<std::string, int> new_iport(target_ip, target_port);
	targetServices[targetServerName].push_back(std::move(new_iport));
	targetSelectIndex[targetServerName] = targetServices[targetServerName].size();
}

/*
 * PortProxyServer模式数据发送客户端完毕
 *
 */
void PortProxyServer::HandleSendComplete(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
}

/*
 * PortProxyServer模式处理连接断开
 *
 */
void PortProxyServer::HandleClose(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
}

/*
 * PortProxyServer模式处理连接出错
 *
 */
void PortProxyServer::HandleError(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
}

/*
 * 获取文件大小
 *
 */
int PortProxyServer::getFileSize(char *file_name)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
    FILE *fp = fopen(file_name, "r");
    if (!fp)
        return -1;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    return size;
}

/*
 * 处理错误http请求，返回错误描述
 * 
 */
void PortProxyServer::HttpError(spTcpConnection &sptcpconn, const std::string &short_msg)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
    std::string &responsecontext = sptcpconn->GetBufferOut();
    responsecontext.clear();
    responsecontext += "HTTP/1.1 200 ok\r\n";
    responsecontext += "Server: Qiu Hai's NetServer/PortProxyServer\r\n";
    responsecontext += "Content-Type: text/html\r\n";
    responsecontext += "Connection: close\r\n";
    std::string responsebody;
    responsebody += "<html><title>请求出错了</title>";
    responsebody += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>";
    responsebody += "<style>body{background-color:#f;font-size:14px;}h1{font-size:60px;color:#eeetext-align:center;padding-top:30px;font-weight:normal;}</style>";
    responsebody += "<body bgcolor=\"ffffff\"><h1>";
    responsebody += short_msg;
    responsebody += "</h1><hr><em> Qiu Hai's NetServer</em>\n</body></html>";
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n\r\n";
    responsecontext.append(responsebody, 0, responsebody.length());
    sptcpconn->SendBufferOut();
}

/*
 * 处理请求并响应
 * 代理两个连接之间进行数据通信函数
 * 
 *
 */
void PortProxyServer::ProxyProcess(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "函数触发，工作端口：%s:%d\n", tcpServerIP_.data(), tcpServerPort_);
	HttpRequestContext &httpReq = sptcpconn->GetReqestBuffer();
	std::string reqServiceName = httpReq.serviceName;
	std::string &ioBuffer = sptcpconn->GetBufferIn();
	std::string target_ip;
	int target_port;
	if(targetSelectIndex.end() == targetSelectIndex.find(reqServiceName) || targetServices.end() == targetServices.find(reqServiceName))
	{
    	LOG(LoggerLevel::ERROR, "没有reqServiceName=%s，工作端口：%s:%d\n", reqServiceName.c_str(), tcpServerIP_.data(), tcpServerPort_);
    	std::cout << "PortProxyServer::ProxyProcess 没有reqServiceName=" << reqServiceName << std::endl;
		HttpError(sptcpconn, "目标服务请求失败，请稍后重试");
	}
	else
	{
		targetSelectIndex[reqServiceName] = (targetSelectIndex[reqServiceName] + 1) % targetServices[reqServiceName].size();
		std::tie(target_ip, target_port) = targetServices[reqServiceName][targetSelectIndex[reqServiceName]];
		int req_socket = socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in addr_serv;
		addr_serv.sin_addr.s_addr = inet_addr(target_ip.c_str());
		addr_serv.sin_family = AF_INET;
		addr_serv.sin_port = htons(target_port);
		int conn_res = connect(req_socket, (sockaddr *)&addr_serv, sizeof(sockaddr));
		if (-1 != conn_res)
		{
    		LOG(LoggerLevel::INFO, "获取代理服务连接成功, 连接sockfd：%d，工作端口：%s:%d\n", req_socket, tcpServerIP_.data(), tcpServerPort_);
			std::cout << "PortProxyServer::ProxyProcess 获取代理服务连接成功, 连接sockfd：" << req_socket << std::endl;
			// 移除基于epoll的tcpserver_对客户端连接的监听，转为由工作线程通过poll函数控制
			loop.RemoveChannelToPoller(sptcpconn->GetChannel()); 
			sptcpconn->requestToOut();
			sptcpconn->sendn(req_socket, sptcpconn->GetBufferOut());
			struct pollfd pollFd[2];
			pollFd[0].fd = sptcpconn->fd();
			pollFd[0].events = POLLRDNORM;
			pollFd[1].fd = req_socket;
			pollFd[1].events = POLLRDNORM;
			int buflen = 3 * 1024 * 1024;
			char *buf = new char[buflen];
			// TCP状态检测，主要是检测断开连接
			struct tcp_info tcpinfo;
			int tcpinfolen = sizeof(tcpinfo);
			while(true)
			{
				while (0 == poll(pollFd, 2, 1000))
					continue;
				int index = 0;
				for (; index < 2; index++)
				{
					// 连接两端任意一端发来数据
					if (pollFd[index].revents & POLLRDNORM)
					{
						break;
					}
				}
				if (index >= 2)
					continue;
				int this_fd = pollFd[index].fd; // 数据来源端
				int other_fd = pollFd[(index ? 0 : 1)].fd; // 数据接收端
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
				// 连接断开，退出for循环
				if (tcpinfo.tcpi_state == TCP_CLOSE_WAIT)
				{
					break;
				}
			}
			delete[] buf;
			close(req_socket);
		}
		else
		{
    		LOG(LoggerLevel::ERROR, "获取代理服务连接失败, 连接sockfd：%d，工作端口：%s:%d\n", req_socket, tcpServerIP_.data(), tcpServerPort_);
			std::cout << "PortProxyServer::ProxyProcess 获取代理服务连接失败, 连接sockfd：" << req_socket << std::endl;
			close(req_socket);
			HttpError(sptcpconn, "目标服务请求失败，请稍后重试");
		}
	}
}



