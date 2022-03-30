
// http网络服务类

// GET /register.do?p={%22username%22:%20%2213917043329%22,%20%22nickname%22:%20%22balloon%22,%20%22password%22:%20%22123%22} HTTP/1.1\r\n
// GET / HTTP/1.1
// Host: baidu.com
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
// Accept-Encoding: gzip, deflate, br
// Accept-Language: zh-CN,zh;q=0.9,en;q=0.8
// Cookie: _bdid_=059a16ee3bef488b9d5212c81e2b688d; Hm_lvt_c58f67ca105d070ca7563b4b14210980=1550223017; _ga=GA1.2.265126182.1550223018; _gid=GA1.2.1797252688.1550223018; Hm_lpvt_c58f67ca105d070ca7563b4b14210980=1550223213; _gat_gtag_UA_124915922_1=1

// HTTP/1.1 200 OK
// Server: nginx/1.13.12
// Date: Fri, 15 Feb 2019 09:57:21 GMT
// Content-Type: text/html; charset=utf-8
// Transfer-Encoding: chunked
// Connection: keep-alive
// Vary: Accept-Encoding
// Vary: Cookie
// X-Frame-Options: SAMEORIGIN
// Set-Cookie: __bqusername=""; Domain=.bigquant.com; expires=Thu, 01-Jan-1970 00:00:00 GMT; Max-Age=0; Path=/
// Access-Control-Allow-Origin: *
// Content-Encoding: gzip

// 200：请求被正常处理
// 204：请求被受理但没有资源可以返回
// 206：客户端只是请求资源的一部分，服务器只对请求的部分资源执行GET方法，相应报文中通过Content-Range指定范围的资源。

// 301：永久性重定向
// 302：临时重定向
// 303：与302状态码有相似功能，只是它希望客户端在请求一个URI的时候，能通过GET方法重定向到另一个URI上
// 304：发送附带条件的请求时，条件不满足时返回，与重定向无关
// 307：临时重定向，与302类似，只是强制要求使用POST方法

// 400：请求报文语法有误，服务器无法识别
// 401：请求需要认证
// 403：请求的对应资源禁止被访问
// 404：服务器无法找到对应资源

// 500：服务器内部错误
// 503：服务器正忙

#pragma once

#include <string>
#include <mutex>
#include <map>
#include <iostream>
#include <functional>
#include <memory>
#include "TcpServer.hpp"
#include "EventLoop.hpp"
#include "TimerManager.hpp"
#include "TcpConnection.hpp"
#include "ThreadPool.hpp"


class HttpServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::shared_ptr<Timer> spTimer;
    HttpServer(EventLoop *loop, const int workThreadNum = 2, ThreadPool *threadPool = NULL, const int loopThreadNum = 0, const int port = 80, TcpServer *shareTcpServer = NULL);
    ~HttpServer();
    int getFileSize(char* file_name);   // 获取文件大小
    void HttpProcess(spTcpConnection &sptcpconn);   // 处理请求并响应
    void HttpError(spTcpConnection &sptcpconn, const int err_num, const std::string &short_msg);
    void Start();   // tcpServer创建所需的事件池子线程并启动，添加tcp服务Channel实例为监听对象

private:
    std::string serviceName_;
    std::mutex mutex_;
    std::map<spTcpConnection, spTimer> timerList_;
    int workThreadNum_;
    int loopThreadNum_;
    int tcpServerPort_;         // tcpServer的EPOLL的服务端口
    TcpServer *tcpserver_;      // 基础网络服务TcpServer
    ThreadPool *threadpool_;    // 线程池
    void HandleMessage(spTcpConnection &sptcpconn);       // HttpServer模式处理收到的请求
    void HandleSendComplete(spTcpConnection &sptcpconn);  // HttpServer模式数据处理发送客户端完毕
    void HandleClose(spTcpConnection &sptcpconn);         // HttpServer模式处理连接断开
    void HandleError(spTcpConnection &sptcpconn);         // HttpServer模式处理连接出错

};

HttpServer::HttpServer(EventLoop *loop, const int workThreadNum, ThreadPool *threadPool, const int loopThreadNum, const int port, TcpServer *shareTcpServer)
    : serviceName_("HttpService"),
      loopThreadNum_(loopThreadNum),
      workThreadNum_(workThreadNum),
      tcpServerPort_(port),
      threadpool_(threadPool ? threadPool : (workThreadNum_ > 0 ? new ThreadPool(workThreadNum_) : NULL)),
      tcpserver_(shareTcpServer ? shareTcpServer : new TcpServer(loop, tcpServerPort_, loopThreadNum))
{
    // 基于TcpServer设置HttpServer服务函数，在TcpServer内触发调用HttpServer的成员函数，类似于信号槽机制
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ReadMessageHandler, std::bind(&HttpServer::HandleMessage, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::SendOverHandler, std::bind(&HttpServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::CloseConnHandler, std::bind(&HttpServer::HandleClose, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ErrorConnHandler, std::bind(&HttpServer::HandleError, this, std::placeholders::_1));
    threadpool_->Start();
    TimerManager::GetTimerManagerInstance()->Start();
}

HttpServer::~HttpServer()
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
 * HttpServer模式处理收到的请求
 * 
 */
void HttpServer::HandleMessage(spTcpConnection &sptcpconn)
{
    // 修改定时器参数
    sptcpconn->GetTimer()->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    if (threadpool_->GetThreadNum() > 0)
    {
        // 已开启线程池，线程池taskQueue_添加任务
        if (false == sptcpconn->GetReqHealthy())
        {
            HttpError(sptcpconn, 400, "Bad request");
            return;
        }
        sptcpconn->SetAsyncProcessing(true);
        // 线程池在此添加任务并唤醒一工作线程执行之
        threadpool_->AddTask([&]()
                            {
                                HttpProcess(sptcpconn);
                                if (!sptcpconn->WillKeepAlive())
                                {
                                    // 短连接，关闭连接
                                    // sptcpconn->HandleClose();
                                }
                            });
    }
    else
    {
        // 没有开启线程池
        if (false == sptcpconn->GetReqHealthy())
        {
            HttpError(sptcpconn, 400, "Bad request");
            return;
        }
        HttpProcess(sptcpconn);
        if (!sptcpconn->WillKeepAlive())
        {
            // sptcpconn->HandleClose();
        }
    }
}

/*
 * HttpServer模式数据发送客户端完毕
 * 
 */
void HttpServer::HandleSendComplete(spTcpConnection &sptcpconn)
{
}

/*
 * HttpServer模式处理连接断开
 * 
 */
void HttpServer::HandleClose(spTcpConnection &sptcpconn)
{
}

/*
 * HttpServer模式处理连接出错
 * 
 */
void HttpServer::HandleError(spTcpConnection &sptcpconn)
{
}

/*
 * http请求处理与响应
 * 
 */
void HttpServer::HttpProcess(spTcpConnection &sptcpconn)
{
    HttpRequestContext &httprequestcontext = sptcpconn->GetReq();
    std::string &responsecontext = sptcpconn->GetBufferOut();
    std::string responsebody;
    std::string errormsg;
    std::string path;
    std::string querystring;
    std::string filetype("text/html"); // 默认资源文件类型
    if ("GET" == httprequestcontext.method)
    {
        ;
    }
    else if ("POST" == httprequestcontext.method)
    {
        ;
    }
    else
    {
        // 对其他方法不支持
        errormsg = "不支持方法：" + httprequestcontext.method + " (Method Not Implemented)";
        HttpError(sptcpconn, 501, errormsg.data());
        return;
    }
    size_t pos = httprequestcontext.url.find("?");
    if (pos != std::string::npos)
    {
        // 请求链接包含?，获取'?'以前的path以及'?'以后的querystring
        path = httprequestcontext.url.substr(0, pos);
        querystring = httprequestcontext.url.substr(pos + 1);
    }
    else
    {
        path = httprequestcontext.url;
    }
    // keepalive判断处理，包含Connection字段
    std::map<std::string, std::string>::const_iterator iter = httprequestcontext.header.find("Connection");
    if (iter != httprequestcontext.header.end())
    {
        // Connection字段值为Keep-Alive则保持长连接
        sptcpconn->SetKeepAlive(iter->second == "Keep-Alive");
    }
    else
    {
        // 不包含Keep-Alive字段，根据协议版本判断是否需要长连接
        if (httprequestcontext.version == "HTTP/1.1")
        {
            sptcpconn->SetKeepAlive(true); // HTTP/1.1默认长连接
        }
        else
        {
            sptcpconn->SetKeepAlive(false); // HTTP/1.0默认短连接
        }
    }
    if ("/" == path)
    {
        // 默认访问index.html页面
        path = wwwRoot + "index.html";
    }
    else if ("/hello" == path)
    {
        // '/hello'处理为以下内容，作为参考
        responsebody = ("hello world");
        responsecontext += httprequestcontext.version + " 200 OK\r\n";
        responsecontext += "Server: Qiu Hai's NetServer/0.1\r\n";
        responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
        if (iter != httprequestcontext.header.end())
        {
            responsecontext += "Connection: " + iter->second + "\r\n";
        }
        responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
        responsecontext += "\r\n";
        responsecontext += responsebody;
        return;
    }
    else
    {
        // 为请求的网页资源加上正确的相对路径前缀
        path = wwwRoot + path;
    }
    size_t point;
    if ((point = path.rfind('.')) != std::string::npos)
    {
        // 判断请求资源文件类型
        filetype = TypeIdentify::getContentType(path.substr(point));
        if(filetype.empty())
        {
            // 无法识别资源文件类型
            filetype = "text/html";
        }
    }
    FILE *fp = NULL;
    if ((fp = fopen(path.c_str(), "rb")) == NULL)
    {
        // 未定位到资源文件
        HttpError(sptcpconn, 404, "Not Found Resource or Service : \"" + httprequestcontext.url + "\"");
        return;
    }
    else
    {
        // 读取并发送请求的资源文件
        std::fstream tmpfile;
        tmpfile.open(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate); // 二进制输入(读取),定位到文件末尾
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
    responsecontext += "Server: QiuHai's NetServer/0.1\r\n";
    responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
    if (iter != httprequestcontext.header.end())
    {
        responsecontext += "Connection: " + iter->second + "\r\n";
    }
    // responsecontext += "Content-Length: " + std::to_string(getFileSize(path.data())) + "\r\n\r\n";
    responsecontext += "Content-Length: " + std::to_string(responsebody.length()) + "\r\n\r\n";
    responsecontext.append(responsebody, 0, responsebody.length());
    sptcpconn->SendBufferOut();
}

/*
 * 处理错误http请求，返回错误描述
 * 
 */
void HttpServer::HttpError(spTcpConnection &sptcpconn, const int err_num, const std::string &short_msg)
{
    std::string &responsecontext = sptcpconn->GetBufferOut();
    if (sptcpconn->GetReq().version.empty())
    {
        responsecontext += "HTTP/1.1 " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    else
    {
        responsecontext += sptcpconn->GetReq().version + " " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    responsecontext += "Server: Qiu Hai's NetServer/0.1\r\n";
    responsecontext += "Content-Type: text/html\r\n";
    responsecontext += "Connection: Keep-Alive\r\n";
    std::string responsebody;
    responsebody += "<html><title>出错了</title>";
    responsebody += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>";
    responsebody += "<style>body{background-color:#f;font-size:14px;}h1{font-size:60px;color:#eeetext-align:center;padding-top:30px;font-weight:normal;}</style>";
    responsebody += "<body bgcolor=\"ffffff\"><h1>";
    responsebody += std::to_string(err_num) + " " + short_msg;
    responsebody += "</h1><hr><em> Qiu Hai's NetServer</em>\n</body></html>";
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
    responsecontext += "\r\n";
    responsecontext.append(responsebody, 0, responsebody.length());
    sptcpconn->SendBufferOut();
}

/*
 * tcpServer创建所需的事件池监听子线程并启动
 * 添加tcpServer的Channel实例为监听对象
 * 
 */
void HttpServer::Start()
{
}

/*
 * 获取文件大小	
 * 
 */
int HttpServer::getFileSize(char* file_name)
{
	FILE *fp=fopen(file_name,"r");
	if(!fp)
		return -1;
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fclose(fp);
	return size;
}

