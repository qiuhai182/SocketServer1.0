
// http网络会话类
//  主要供外部HttpServer类内调用
//  对一个网络连接TcpConnection的请求进行解析、资源分析与处理

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
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include "Resource.hpp"
#include "TypeIdentify.hpp"

#define BUFSIZE 4096

// http请求信息结构
typedef struct HttpRequestContext
{
    std::string method;
    std::string url;
    std::string version;
    std::map<std::string, std::string> header;
    std::string body;
} _HttpRequestContext;

// http响应信息结构
typedef struct HttpResponseContext
{
    std::string version;
    std::string statecode;
    std::string statemsg;
    std::map<std::string, std::string> header;
    std::string body;
} _HttpResponseContext;

class HttpSession
{
private:
    bool keepalive_; // 长连接标志

public:
    HttpSession();
    ~HttpSession();
    bool ParseHttpRequest(std::string &s, HttpRequestContext &httprequestcontext);  // 解析http请求信息
    bool ParseHttpRequest(char *s, int msgLength, HttpRequestContext &httprequestcontext);  // 解析http请求信息
    void HttpProcess(const HttpRequestContext &httprequestcontext, std::string &responsecontext);   // 处理接收到的请求信息
    void HttpError(const int err_num, const std::string &short_msg, const HttpRequestContext &httprequestcontext, std::string &responsecontext);
    bool KeepAlive();   // 保持长连接

};

HttpSession::HttpSession()
    : keepalive_(true)
{
}

HttpSession::~HttpSession()
{
}

/*
 * 获取文件大小	
 * 
 */
int getFileSize(char* file_name)
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
 * 解析http请求信息
 * 
 */
bool HttpSession::ParseHttpRequest(std::string &msg, HttpRequestContext &httprequestcontext)
{
    std::string crlf("\r\n"), crlfcrlf("\r\n\r\n");
    size_t prev = 0, next = 0, pos_colon;
    std::string key, value;
    bool parseresult = false;
    //TODO以下解析可以改成状态机，解决一次收Http报文不完整问题
    if ((next = msg.find(crlf, prev)) != std::string::npos)
    {
        std::string first_line(msg.substr(prev, next - prev));
        prev = next;
        std::stringstream sstream(first_line);
        sstream >> (httprequestcontext.method);
        sstream >> (httprequestcontext.url);
        sstream >> (httprequestcontext.version);
    }
    else
    {
        std::cout << "error received message：" << msg << std::endl;
        std::cout << "Error in httpParser: http_request_line isn't complete!" << std::endl;
        parseresult = false;
        msg.clear();
        return parseresult;
        // TODO 可以临时存起来，凑齐了再解析
    }
    size_t pos_crlfcrlf = 0;
    if ((pos_crlfcrlf = msg.find(crlfcrlf, prev)) != std::string::npos)
    {
        while (prev != pos_crlfcrlf)
        {
            next = msg.find(crlf, prev + 2);
            pos_colon = msg.find(":", prev + 2);
            key = msg.substr(prev + 2, pos_colon - prev - 2);
            value = msg.substr(pos_colon + 2, next - pos_colon - 2);
            prev = next;
            httprequestcontext.header.insert(std::pair<std::string, std::string>(key, value));
        }
    }
    else
    {
        std::cout << "Error in httpParser: http_request_header isn't complete!" << std::endl;
        parseresult = false;
        msg.clear();
        return parseresult;
    }
    httprequestcontext.body = msg.substr(pos_crlfcrlf + 4);
    parseresult = true;
    msg.clear();
    return parseresult;
}

/*
 * 解析http请求信息
 * 
 */
bool HttpSession::ParseHttpRequest(char *msg, int msgLength, HttpRequestContext &httprequestcontext)
{
    const char *crlf = "\r\n";
    const char *crlfcrlf = "\r\n\r\n";
    bool parseresult = false;
    char *preFind = msg, *nextFind = NULL, *pos_colon = nullptr;
    std::string key, value;
    char *const pos_crlfcrlf = strstr(preFind, crlfcrlf);
    char buffer[BUFSIZE];
    // TODO 以下解析可以改成状态机，解决一次收Http报文不完整问题
    if(nextFind = strstr(preFind, crlf))
    {
        memcpy(buffer, preFind, nextFind - preFind);
        std::string first_line(buffer, nextFind - preFind);
        preFind = nextFind + 2;
        std::stringstream sstream(first_line);
        sstream >> (httprequestcontext.method);
        sstream >> (httprequestcontext.url);
        sstream >> (httprequestcontext.version);
    }
    else
    {
        std::cout << "接收到信息：" << msg << std::endl;
        std::cout << "Error in httpParser: http_request_line 不完整!" << std::endl;
        parseresult = false;
        return parseresult;
        //可以临时存起来，凑齐了再解析
    }
    if(pos_crlfcrlf)
    {
        while(pos_crlfcrlf != (nextFind = strstr(preFind, crlf)))
        {
            // 仍在查询请求头部分
            if(nextFind)
            {
                pos_colon = strstr(preFind + 2, ":");
                memcpy(buffer, preFind, pos_colon - preFind);
                key.clear();
                key.append(buffer, pos_colon - preFind);
                memcpy(buffer, pos_colon + 2, nextFind - (pos_colon + 2));
                value.clear();
                value.append(buffer, nextFind - (pos_colon + 2));
                preFind = nextFind + 2;
                httprequestcontext.header.insert(std::pair<std::string, std::string>(key, value));
            }
            else
                break;
        }
    }
    else
    {
        std::cout << "接收到信息：" << msg << std::endl;
        std::cout << "Error in httpParser: http_request_header 不完整!" << std::endl;
        parseresult = false;
        return parseresult;
    }
    std::string conLen = httprequestcontext.header["Content-Length"];
    int contentLength = conLen.empty() ? msgLength - (pos_crlfcrlf + 4 - msg) : atoi(conLen.c_str());
    memcpy(buffer, pos_crlfcrlf + 4, contentLength);
    httprequestcontext.body.clear();
    httprequestcontext.body.append(buffer, 0, contentLength);
    parseresult = true;
    return parseresult;
}

/*
 * http请求处理与响应函数
 * 
 */
void HttpSession::HttpProcess(const HttpRequestContext &httprequestcontext, std::string &responsecontext)
{
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
        HttpError(501, errormsg.data(), httprequestcontext, responsecontext);
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
        keepalive_ = (iter->second == "Keep-Alive");
    }
    else
    {
        // 不包含Keep-Alive字段，根据协议版本判断是否需要长连接
        if (httprequestcontext.version == "HTTP/1.1")
        {
            keepalive_ = true; // HTTP/1.1默认长连接
        }
        else
        {
            keepalive_ = false; // HTTP/1.0默认短连接
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
        HttpError(404, "Not Found", httprequestcontext, responsecontext);
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

}

/*
 * 处理错误http请求，返回错误描述
 * 
 */
void HttpSession::HttpError(const int err_num, const std::string &short_msg, const HttpRequestContext &httprequestcontext, std::string &responsecontext)
{
    responsecontext.clear();
    if (httprequestcontext.version.empty())
    {
        responsecontext += "HTTP/1.1 " + std::to_string(err_num) + " " + short_msg + "\r\n";
    }
    else
    {
        responsecontext += httprequestcontext.version + " " + std::to_string(err_num) + " " + short_msg + "\r\n";
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
}

/*
 * TODO
 * 
 */
bool HttpSession::KeepAlive()
{
    return keepalive_;
}
