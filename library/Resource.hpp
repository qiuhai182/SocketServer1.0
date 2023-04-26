
// 定义项目资源访问路径、通用变量

#pragma once

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include <string>

// '/resource'目录相对位于'/SocketServer/service/XXService/out/'目录下的可执行文件server的相对访问路径
const std::string dataRoot = "../../../resource/";

// 日志目录
const std::string logPath = "../../../logs/";

// 网站资源根目录
const std::string wwwRoot = dataRoot + "www/";
const std::string imgRoot = dataRoot + "images/";

#endif