
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
#include "LogServer.hpp"
#include "ThreadPool.hpp"
#include "TypeIdentify.hpp"
#include "TcpConnection.hpp"

class HttpServer
{
public:
    HttpServer(EventLoop *loop, const int workThreadNum = 2, ThreadPool *threadPool = NULL, const int loopThreadNum = 0, const int port = 80, TcpServer *shareTcpServer = NULL);
    ~HttpServer();

private:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    std::string serviceName_;
    int workThreadNum_;     // 默认为0，若有值则自行管理创建销毁一个线程池
    int loopThreadNum_;     // 默认为0，若有值则自行管理创建销毁一个TcpServerr对象，该对象也会自行管理eventLoopThreadPool线程池
    int tcpServerPort_;                           // tcpServer的EPOLL的服务端口
    TcpServer *tcpserver_;                        // 基础网络服务TcpServer
    ThreadPool *threadpool_;                      // 线程池
    int getFileSize(char *file_name);             // 获取文件大小
    void HttpProcess(spTcpConnection &sptcpconn); // 处理请求并响应
    // 处理错误http请求，返回错误描述
    void HttpError(spTcpConnection &sptcpconn, const int err_num, const std::string &short_msg);
    void SendResource(spTcpConnection &sptcpconn, const std::string &filePath); // 发送请求的资源到客户端
    void HandleMessage(spTcpConnection &sptcpconn);                             // HttpServer模式处理收到的请求
    void HandleSendComplete(spTcpConnection &sptcpconn);                        // HttpServer模式数据处理发送客户端完毕
    void HandleClose(spTcpConnection &sptcpconn);                               // HttpServer模式处理连接断开
    void HandleError(spTcpConnection &sptcpconn);                               // HttpServer模式处理连接出错

};

HttpServer::HttpServer(EventLoop *loop, const int workThreadNum, ThreadPool *threadPool, const int loopThreadNum, const int port, TcpServer *shareTcpServer)
    : serviceName_("HttpService"),
      loopThreadNum_(loopThreadNum),
      workThreadNum_(workThreadNum),
      tcpServerPort_(port),
      threadpool_(threadPool ? threadPool : (workThreadNum_ > 0 ? new ThreadPool(workThreadNum_) : NULL)),
      tcpserver_(shareTcpServer ? shareTcpServer : new TcpServer(loop, tcpServerPort_, loopThreadNum))
{
    // 基于TcpServer设置HttpServer服务函数，在TcpServer内注册HttpServer的成员函数等待Channel绑定
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ReadMessageHandler, std::bind(&HttpServer::HandleMessage, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::SendOverHandler, std::bind(&HttpServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::CloseConnHandler, std::bind(&HttpServer::HandleClose, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::ErrorConnHandler, std::bind(&HttpServer::HandleError, this, std::placeholders::_1));
    tcpserver_->RegisterHandler(serviceName_, TcpServer::HttpHandler, std::bind(&HttpServer::HttpProcess, this, std::placeholders::_1));
    if (tcpServerPort_)
        threadpool_->Start();
}

HttpServer::~HttpServer()
{
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
    if (workThreadNum_)
    {
        delete threadpool_;
    }
    if (tcpServerPort_)
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
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
    // 修改定时器参数
    sptcpconn->GetTimer()->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    sptcpconn->SetAsyncProcessing(false);
    if (false == sptcpconn->GetReqHealthy())
    {
        HttpError(sptcpconn, 400, "Bad request");
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
                                LOG(LoggerLevel::INFO, "工作线程即将执行sptcpconn的绑定函数，此时sptcpconn->IsDisconnected()：%s，sockfd：%d\n", sptcpconn->IsDisconnected() ? "true" : "false", sptcpconn->fd());
                                if(sptcpconn->IsDisconnected())
                                {
                                    LOG(LoggerLevel::INFO, "工作线程即将执行sptcpconn的绑定函数，此时sptcpconn已关闭，不作处理，sockfd：%d\n", sptcpconn->fd());
                                }
                                else
                                {
                                    try
                                    {
                                        LOG(LoggerLevel::INFO, "%s\n", "工作线程执行sptcpconn的绑定函数");
                                        sptcpconn->GetReqHandler()(sptcpconn);
                                    }
                                    catch(std::bad_function_call)
                                    {
                                        LOG(LoggerLevel::ERROR, "工作线程执行sptcpconn的绑定函数报错：std::bad_function_call，连接绑定函数异常，sockfd：%d\n", sptcpconn->fd());
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
            LOG(LoggerLevel::ERROR, "执行sptcpconn的绑定函数报错：std::bad_function_call，连接绑定函数异常，sockfd：%d\n", sptcpconn->fd());
        }
    }
}

/*
 * http请求处理与响应
 *
 */
void HttpServer::HttpProcess(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "开始处理一个TcpConnection连接的Http请求，连接sockfd：%d\n", sptcpconn->fd());
    HttpRequestContext &httprequestcontext = sptcpconn->GetReqestBuffer();
    std::string &responsecontext = sptcpconn->GetBufferOut(); // 存储响应头+响应内容
    std::string responsebody;                                 // 暂存响应内容
    std::string path;                                         // 请求的资源url
    std::string querystring;                                  // 请求url的'?'后的信息
    std::string filetype("text/html");                        // 默认资源文件类型
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
        std::string errormsg = "不支持方法：" + httprequestcontext.method + " (Method Not Implemented)";
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
        responsecontext += "Server: Qiu Hai's NetServer/HttpService\r\n";
        responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
        if (iter != httprequestcontext.header.end())
        {
            responsecontext += "Connection: " + iter->second + "\r\n";
        }
        responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n\r\n";
        responsecontext += responsebody;
        sptcpconn->SendBufferOut();
        return;
    }
    else
    {
        // 为请求的网页资源加上正确的相对路径前缀
        path = wwwRoot + path;
    }
    SendResource(sptcpconn, path);
}

/*
 * 发送请求的资源到客户端
 *
 */
void HttpServer::SendResource(spTcpConnection &sptcpconn, const std::string &filePath)
{
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
    std::string filetype = TypeIdentify::getContentTypeByPath(filePath);
    HttpRequestContext &httprequestcontext = sptcpconn->GetReqestBuffer();
    if (filetype.empty())
    {
        // 未知的资源类型
        LOG(LoggerLevel::ERROR, "未知的资源类型：%s(类型为：%s)\n", filePath, filetype);
        size_t npos = filePath.rfind('/');
        HttpError(sptcpconn, 404, "Not Found Resource : \"" + filePath.substr(npos + 1) + "\" ,unknown file-type");
        return;
    }
    std::string &responsecontext = sptcpconn->GetBufferOut(); // 存储响应头+响应内容
    std::string responsebody;                                 // 暂存响应内容
    FILE *fp = NULL;
    if ((fp = fopen(filePath.c_str(), "rb")) == NULL)
    {
        // 未定位到资源文件
        LOG(LoggerLevel::INFO, "未寻到资源：%s\n", filePath);
        size_t npos = filePath.rfind('/');
        HttpError(sptcpconn, 404, "Not Found Resource : \"" + filePath.substr(npos + 1) + "\" ");
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
    LOG(LoggerLevel::INFO, "即将发送文件：%s，文件类型：%s\n", filePath, filetype);
    sptcpconn->SendBufferOut();
}

/*
 * 处理错误http请求，返回错误描述
 * 该函数不同于HandleError，该函数不能被Channel调用
 *
 */
void HttpServer::HttpError(spTcpConnection &sptcpconn, const int err_num, const std::string &short_msg)
{
    LOG(LoggerLevel::INFO, "%s，sockfd：%d\n", "函数触发", sptcpconn->fd());
    std::string &responsecontext = sptcpconn->GetBufferOut();
    responsecontext.clear();
    std::string responsebody;
    responsebody += "<html><title>出错了</title>";
    responsebody += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>";
    responsebody += "<style>body{background-color:#f;font-size:14px;}h1{font-size:60px;color:#eeetext-align:center;padding-top:30px;font-weight:normal;}</style>";
    responsebody += "<body bgcolor=\"ffffff\"><h1>";
    responsebody += std::to_string(err_num) + " " + short_msg;
    responsebody += "</h1><hr><em> Qiu Hai's NetServer</em>\n</body></html>";
    if (sptcpconn->GetReqestBuffer().version.empty())
    {
        responsecontext += "HTTP/1.1 " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    else
    {
        responsecontext += sptcpconn->GetReqestBuffer().version + " " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    responsecontext += "Server: Qiu Hai's NetServer/HttpService\r\n";
    responsecontext += "Content-Type: text/html\r\n";
    responsecontext += "Connection: Keep-Alive\r\n";
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n\r\n";
    responsecontext.append(responsebody, 0, responsebody.length());
    sptcpconn->SendBufferOut();
}

/*
 * HttpServer模式数据发送客户端完毕
 *
 */
void HttpServer::HandleSendComplete(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
}

/*
 * HttpServer模式处理连接断开
 *
 */
void HttpServer::HandleClose(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
}

/*
 * HttpServer模式处理连接出错
 *
 */
void HttpServer::HandleError(spTcpConnection &sptcpconn)
{
    LOG(LoggerLevel::INFO, "%s\n", "函数触发");
}

/*
 * 获取文件大小
 *
 */
int HttpServer::getFileSize(char *file_name)
{
    FILE *fp = fopen(file_name, "r");
    if (!fp)
        return -1;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    return size;
}
