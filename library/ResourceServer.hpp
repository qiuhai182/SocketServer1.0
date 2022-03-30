
// 文件资源获取及上传服务
//  与HttpServer的Get/Post与Put方法获取/上传资源文件不同，
//  HttpServer获取资源将资源路径写在url内，
//  ResourceServer的网络请求url仅用于服务定位（服务名、函数名），
//  获取、上传资源文件的一切信息位于header、content内

#pragma once

#include <string>
// #include <mutex>
// #include <map>
// #include <iostream>
#include <functional>
#include <memory>
#include "TcpServer.hpp"
// #include "EventLoop.hpp"
#include "TimerManager.hpp"
#include "TcpConnection.hpp"
// #include "HttpSession.hpp"
// #include "ThreadPool.hpp"
#include "Timer.hpp"


class ResourceServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::shared_ptr<Timer> spTimer;
    ResourceServer();
    ~ResourceServer();
    void Start();
private:
    std::string serviceName_;

};


ResourceServer::ResourceServer()
    :  serviceName_("ResourceService")
{
    ;
}

