
// 文件资源获取及上传服务
//  与HttpServer的Get/Post与Put方法获取/上传资源文件相似，
//  HttpServer仅需将请求的资源写在url内，以/www目录的相对位置访问资源，
//  而ResourceServer的网络请求url可用于服务定位（服务名、函数名）及资源访问，
//  以/resource目录的相对位置访问资源，
//  header、content内可传递信息，content内传递的信息为json格式数据

#pragma once

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <memory>
#include <functional>
#include "Timer.hpp"
#include "Resource.hpp"
#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "ThreadPool.hpp"
#include "TypeIdentify.hpp"
#include "TcpConnection.hpp"
#include "jsoncpp/json.h"


class ResourceServer
{
public:
    ResourceServer(EventLoop *loop, const int workThreadNum = 2, ThreadPool *threadPool = NULL, const int loopThreadNum = 0, const int port = 80, TcpServer *shareTcpServer = NULL);
    ~ResourceServer();

private:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    std::string serviceName_;
    std::mutex mutex_;
    int workThreadNum_;
    int loopThreadNum_;
    int tcpServerPort_;         // tcpServer的EPOLL的服务端口
    TcpServer *tcpserver_;      // 基础网络服务TcpServer
    ThreadPool *threadpool_;    // 线程池
    int getFileSize(char* file_name);                       // 获取文件大小
    void GetImageResource(spTcpConnection &sptcpconn);      // 获取图片资源文件
    void SendResource(spTcpConnection &sptcpconn, const std::string &filePath); // 发送请求的资源到客户端
    void HttpError(spTcpConnection &sptcpconn, const std::string &short_msg);  // 解析请求内容失败
    void HandleMessage(spTcpConnection &sptcpconn);         // ResourceServer模式处理收到的请求
    void HandleSendComplete(spTcpConnection &sptcpconn);    // ResourceServer模式数据处理发送客户端完毕
    void HandleClose(spTcpConnection &sptcpconn);           // ResourceServer模式处理连接断开
    void HandleError(spTcpConnection &sptcpconn);           // ResourceServer模式处理连接出错

};

ResourceServer::ResourceServer(EventLoop *loop, const int workThreadNum, ThreadPool *threadPool, const int loopThreadNum, const int port, TcpServer *shareTcpServer)
    :  serviceName_("ResourceService"),
      loopThreadNum_(loopThreadNum),
      workThreadNum_(workThreadNum),
      tcpServerPort_(port),
      threadpool_(threadPool ? threadPool : (workThreadNum_ > 0 ? new ThreadPool(workThreadNum_) : NULL)),
      tcpserver_(shareTcpServer ? shareTcpServer : new TcpServer(loop, tcpServerPort_, loopThreadNum))
{
    // 基于TcpServer设置ResourceServer服务函数，在TcpServer内触发调用ResourceServer的成员函数，类似于信号槽机制
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ReadMessageHandler, std::bind(&ResourceServer::HandleMessage, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::SendOverHandler, std::bind(&ResourceServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::CloseConnHandler, std::bind(&ResourceServer::HandleClose, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ErrorConnHandler, std::bind(&ResourceServer::HandleError, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, "GetImageResource", std::bind(&ResourceServer::GetImageResource, this, std::placeholders::_1));
    if(tcpServerPort_) threadpool_->Start();
}

ResourceServer::~ResourceServer()
{
    if(workThreadNum_)
    {
        delete threadpool_;
    }
    if(tcpServerPort_)
    {
        delete tcpserver_;
    }
}

/*
 * ResourceServer模式处理收到的请求
 * 
 */
void ResourceServer::HandleMessage(spTcpConnection &sptcpconn)
{
    std::cout << "输出测试：ResourceServer::HandleMessage " << std::endl;
    // 修改定时器参数
    sptcpconn->GetTimer()->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    if (false == sptcpconn->GetReqHealthy())
    {
        Json::Value resMsg;
        resMsg["resCode"] = 400;
        resMsg["aqlRes"] = "Bad request'";
        HttpError(sptcpconn, resMsg.toStyledString());
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
                                sptcpconn->GetReqHandler()(sptcpconn);
                                sptcpconn->SetAsyncProcessing(false);
                            });
    }
    else
    {
        // 没有开启线程池，执行动态绑定的处理函数
        sptcpconn->GetReqHandler()(sptcpconn);
    }
}

/*
 * 处理错误http请求，返回错误描述
 * 
 */
void ResourceServer::HttpError(spTcpConnection &sptcpconn, const std::string &short_msg)
{
    std::cout << "输出测试：ResourceServer::HttpError " << std::endl;
    std::string &responsecontext = sptcpconn->GetBufferOut();
    responsecontext.clear();
    responsecontext += "HTTP/1.1 200 ok\r\n";
    responsecontext += "Server: Qiu Hai's NetServer/ResourceServer\r\n";
    responsecontext += "Content-Type: application/json\r\n";
    responsecontext += "Connection: close\r\n";
    responsecontext += "Content-Length: " + std::to_string(short_msg.size()) + "\r\n\r\n";
    responsecontext.append(short_msg, 0, short_msg.length());
    sptcpconn->SendBufferOut();
}

/*
 * ResourceServer模式数据处理发送客户端完毕
 * 
 */
void ResourceServer::HandleSendComplete(spTcpConnection &sptcpconn)
{
    std::cout << "输出测试：ResourceServer::HandleSendComplete " << std::endl;
}

/*
 * ResourceServer模式处理连接断开
 * 
 */
void ResourceServer::HandleClose(spTcpConnection &sptcpconn)
{
    std::cout << "输出测试：ResourceServer::HandleClose " << std::endl;
    ;
}

/*
 * ResourceServer模式处理连接出错
 * 
 */
void ResourceServer::HandleError(spTcpConnection &sptcpconn)
{
    std::cout << "输出测试：ResourceServer::HandleError " << std::endl;
    ;
}

/*
 * 获取文件大小	
 * 
 */
int ResourceServer::getFileSize(char* file_name)
{
	FILE *fp=fopen(file_name,"r");
	if(!fp)
		return -1;
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fclose(fp);
	return size;
}

/*
 * 发送请求的资源到客户端
 * 
 */
void ResourceServer::SendResource(spTcpConnection &sptcpconn, const std::string &filePath)
{    
    std::cout << "输出测试：ResourceServer::SendResource " << std::endl;
    std::string filetype = TypeIdentify::getContentTypeByPath(filePath);
    Json::Value resMsg;
    if(filetype.empty())
    {
        // 未知的资源类型
        std::cout << "输出测试：ResourceServer::SendResource 未知的资源类型：" << filePath << " (" << filetype << ")" << std::endl;
        resMsg["resCode"] = 404;
        size_t npos = filePath.rfind('/');
        resMsg["aqlRes"] = "not found " + filePath.substr(npos + 1) + " ,unknown file-type";
        HttpError(sptcpconn, resMsg.toStyledString());
        return;
    }
    HttpRequestContext &httprequestcontext = sptcpconn->GetReqestBuffer();
    std::string &responsecontext = sptcpconn->GetBufferOut();   // 存储响应头+响应内容
    std::string responsebody;   // 暂存响应内容
    FILE *fp = NULL;
    if ((fp = fopen(filePath.c_str(), "rb")) == NULL)
    {
        // 未定位到资源文件
        resMsg["resCode"] = 404;
        size_t npos = filePath.rfind('/');
        resMsg["aqlRes"] = "not found " + filePath.substr(npos + 1);
        HttpError(sptcpconn, resMsg.toStyledString());
        return;
    }
    else
    {
        // 读取并发送请求的资源文件
        std::fstream tmpfile;
        tmpfile.open(filePath.c_str(), std::ios::in | std::ios::binary | std::ios::ate); // 二进制输入(读取),定位到文件末尾
        if (tmpfile.is_open())
        {
            size_t length = tmpfile.tellg(); // 获取文件大小
            tmpfile.seekg(0, std::ios::beg); // 定位到文件开始
            char data[BUFSIZE];
            while (length >= BUFSIZE)
            {
                tmpfile.read(data, BUFSIZE);
                responsebody.append(data, BUFSIZE);
                length -= BUFSIZE;
            }
            if (length > 0)
            {
                tmpfile.read(data, length);
                responsebody.append(data, length);
                length = 0;
            }
        }
        fclose(fp);
    }
    responsecontext += httprequestcontext.version + " 200 OK\r\n";
    responsecontext += "Server: QiuHai's NetServer/ResourceService\r\n";
    responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
    // keepalive判断处理，包含Connection字段
    std::map<std::string, std::string>::const_iterator iter = httprequestcontext.header.find("Connection");
    if (iter != httprequestcontext.header.end())
    {
        responsecontext += "Connection: " + iter->second + "\r\n";
    }
    // responsecontext += "Content-Length: " + std::to_string(getFileSize(filePath.data())) + "\r\n\r\n";
    responsecontext += "Content-Length: " + std::to_string(responsebody.length()) + "\r\n\r\n";
    responsecontext.append(responsebody, 0, responsebody.length());
    std::cout << "输出测试：ResourceServer::SendResource 即将发送文件：" << filePath << " 类型为：" << filetype << std::endl;
    sptcpconn->SendBufferOut();
}

/*
 * 获取图片资源文件
 * 
 */
void ResourceServer::GetImageResource(spTcpConnection &sptcpconn)
{
    // 修改定时器参数
    std::cout << "输出测试：ResourceServer::GetImageResource 开始处理一个TcpConnection连接的Http请求，连接sockfd：" << sptcpconn->fd() << std::endl;
    sptcpconn->GetTimer()->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    Json::Reader reader;
    Json::Value jsonBody;
    if (reader.parse(sptcpconn->GetReqestBuffer().body.data(), jsonBody))  // reader将Json字符串解析到root，root将包含Json里所有子元素  
    {  
        std::string imageName = jsonBody["imageName"].asString();
        std::string path = imgRoot + imageName;
        SendResource(sptcpconn, path);
    }
    else
    {
        Json::Value resMsg;
        resMsg["resCode"] = 400;
        resMsg["aqlRes"] = "not found [string] for 'imageName'";
        HttpError(sptcpconn, resMsg.toStyledString());
    }
}



