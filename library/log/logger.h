#pragma once

// Logger类，负责日志存储

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#include <iostream>
#include <queue>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>

#define BUFSIZE 8 * 1024 * 1024     // 8MB
#define LOGLINESIZE 4096            // 4KB
#define MEM_LIMIT 512 * 1024 * 1024 // 512MB

class Logger;

// 仿函数，初始化日志类，每一依次调用都生成新的日志类实例
#define LOG_INIT(logdir, lev)                     \
    do                                            \
    {                                             \
        Logger::GetInstance()->Init(logdir, lev); \
    } while (0)

// 仿函数，日志写入
#define LOG(level, fmt, ...)                                                                          \
    do                                                                                                \
    {                                                                                                 \
        if (Logger::GetInstance()->GetLevel() <= level)                                               \
        {                                                                                             \
            Logger::GetInstance()->Append(level, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__); \
        }                                                                                             \
    } while (0)

// 日志类型
enum LoggerLevel
{
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// const char * LevelString[5] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

// 缓冲区类
class LogBuffer
{
public:
    enum BufState // 缓冲区状态
    {
        FREE = 0, // FREE 空闲状态 可写入日志,
        FLUSH = 1 // FLUSH 待写入或正在写入文件
    };
    LogBuffer(int size = BUFSIZE);
    ~LogBuffer();
    int Getusedlen() const // 已使用缓冲区长度
    {
        return usedlen;
    }
    int GetAvailLen() const // 缓冲区剩余可写长度
    {
        return bufsize - usedlen;
    }
    int GetState() const // 获取缓冲区状态
    {
        return state;
    }
    void SetState(BufState s) // 设置缓冲区状态
    {
        state = s;
    }
    void append(const char *logline, int len); // 添加数据到缓冲区
    void FlushToFile(FILE *fp);                // 写入缓冲区数据到文件

private:
    char *logbuffer;  // log缓冲区
    uint32_t bufsize; // log缓冲区总大小
    uint32_t usedlen; // log缓冲区used长度
    int state;        // 缓冲区状态
};

class Logger
{
private:
    int level; // 日志等级
    FILE *fp;  // 打开的日志文件指针
    // LogBuffer *currentlogbuffer; // 当前使用的缓冲区
    // std::unordered_map<std::thread::id, LogBuffer *> threadbufmap;
    std::map<std::thread::id, LogBuffer *> threadbufmap;
    int buftotalnum; // 缓冲区总数
    std::mutex mtx;
    std::mutex flushmtx;
    std::mutex freemtx;
    std::condition_variable flushcond;
    std::queue<LogBuffer *> flushbufqueue; // flush队列
    std::queue<LogBuffer *> freebufqueue;  // FREE队列
    std::thread flushthread;
    bool start;           // flushthread 状态
    char save_ymdhms[64]; // save_ymdhms数组，保存年月日时分秒以便复用
    // std::hash<std::thread::id> tid_hash;

public:
    Logger();
    ~Logger();
    void Init(const char *logdir, LoggerLevel lev); // 初始化
    static Logger *GetInstance()                    // 单例模式获取指针
    {
        static Logger logger;
        return &logger;
    }
    int GetLevel() const // 获取日志等级
    {
        return level;
    }
    void Append(int level, const char *file, int line, const char *func, const char *fmt, ...); //写日志__FILE__, __LINE__, __func__,
    void Flush();                                                                               // 写入数据到文件
};

#endif //_LOGGER_H_