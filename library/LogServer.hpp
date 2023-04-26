#pragma once

// Logger类，负责日志存储

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

#define BUFSIZE 1024            // 1MB，每个缓冲区类已分配的用于缓存日志的内存大小
#define LOGLINESIZE 1024        // 1MB，每次调用添加单行日志函数，可写的单次数据量最大限制
#define MEM_LIMIT 1024 * 1024    // 512MB，总缓冲区可用内存大小

class Logger;

// 仿函数，初始化日志类，每一依次调用都生成新的日志类实例
#define LOG_INIT(logdir)                          \
    do                                            \
    {                                             \
        Logger::GetInstance()->Init(logdir);      \
    } while (0)

// 仿函数，日志写入
#define LOG(level, fmt, ...)                                                                          \
    do                                                                                                \
    {                                                                                                 \
        Logger::GetInstance()->Append(level, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__);     \
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

const char *LevelString[5] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

// 缓冲区类
class LogBuffer
{
public:
    enum BufState // 缓冲区状态
    {
        FREE = 0, // FREE 空闲状态 可写入日志,
        FLUSH = 1 // FLUSH 待写入或正在写入文件
    };
    // 构造函数
    LogBuffer(int size = BUFSIZE) : bufsize(size),
                                    usedlen(0),
                                    state(BufState::FREE)
    {
        logbuffer = new char[bufsize];
        if (logbuffer == nullptr)
        {
            std::cerr << "mem alloc fail: new char!" << std::endl;
        }
    }
    ~LogBuffer() // 析构函数
    {
        if (logbuffer != nullptr)
        {
            delete[] logbuffer;
        }
    }
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
    void append(const char *logline, int len) // 添加数据到缓冲区
    {
        memcpy(logbuffer + usedlen, logline, len);
        usedlen += len;
    }
    void FlushToFile(FILE *fp) // 写入缓冲区数据到文件
    {
        std::cout << "LogBuffer::FlushToFile 函数触发" << std::endl;
        uint32_t wt_len = fwrite(logbuffer, 1, usedlen, fp);
        if (wt_len != usedlen)
        {
            std::cerr << "fwrite fail!" << std::endl;
        }
        usedlen = 0;
        fflush(fp);
    }

private:
    char *logbuffer;  // log缓冲区
    uint32_t bufsize; // log缓冲区总大小
    uint32_t usedlen; // log缓冲区used长度
    int state;        // 缓冲区状态
};

class Logger
{
private:
    FILE *fp;  // 打开的日志文件指针
    // LogBuffer *currentlogbuffer;    // 当前使用的日志缓冲区，该方式并发性能低，改为每个调用线程一个缓冲区
    // std::unordered_map<std::thread::id, LogBuffer *> threadbufmap; // 为每个首次调用LOG函数的线程生成一个日志缓冲区
    std::map<std::thread::id, LogBuffer *> threadbufmap; // 为每个首次调用LOG函数的线程生成一个日志缓冲区
    int buftotalnum;    // 处于使用中的缓冲区总数
    std::mutex mtx;
    std::mutex flushmtx;
    std::mutex freemtx;
    std::condition_variable flushcond;
    std::queue<LogBuffer *> flushbufqueue; // flush队列，正在写入文件的缓冲区队列
    std::queue<LogBuffer *> freebufqueue;  // FREE队列，不在写入文件状态的缓冲区队列
    std::thread flushthread; // 工作线程
    bool start;           // 工作线程状态，init后置为true，若再置为false则工作线程停止运行
    char save_ymdhms[64]; // save_ymdhms数组，保存年月日时分秒以便复用
    // std::hash<std::thread::id> tid_hash;

public:
    Logger();
    ~Logger();
    void Init(const char *logdir); // 初始化
    static Logger *GetInstance()                    // 单例模式获取指针
    {
        static Logger logger;
        return &logger;
    }
    void Append(int level, const char *file, int line, const char *func, const char *fmt, ...); // 写日志__FILE__, __LINE__, __func__,
    void Flush();                                                                               // 写入数据到文件，线程回调函数
};

/*
 * 日志类构造函数
 *
 */
Logger::Logger(/* args */) : fp(nullptr),
                            //  currentlogbuffer(nullptr),
                             buftotalnum(0),
                             start(false)
{
}

/*
 * 日志类析构函数
 *
 */
Logger::~Logger()
{
    std::cout << "Logger::~Logger 函数触发" << std::endl;
    //最后的日志缓冲区push入队列
    {
        std::lock_guard<std::mutex> lock(mtx);
        // std::unordered_map<std::thread::id, LogBuffer *>::iterator iter;
        std::map<std::thread::id, LogBuffer *>::iterator iter;
        for (iter = threadbufmap.begin(); iter != threadbufmap.end(); ++iter)
        {
            // 所有日志置为flush状态待写入
            iter->second->SetState(LogBuffer::BufState::FLUSH);
            {
                std::lock_guard<std::mutex> lock2(flushmtx);
                flushbufqueue.push(iter->second);
            }
        }
    }
    flushcond.notify_one(); // 工作线程启动，写入所有日志
    start = false; // 准备停止工作线程，当flushbufqueue为空且start为false时工作线程即停止
    flushcond.notify_one(); // 若线程处于wait状态则唤醒
    if (flushthread.joinable())
        flushthread.join(); // 阻塞等待回收工作线程，直至工作线程写完所有日志并停止运行
    if (fp != nullptr)
        fclose(fp);
    // 清理free状态的缓冲区
    while (!freebufqueue.empty())
    {
        LogBuffer *p = freebufqueue.front();
        freebufqueue.pop();
        delete p;
    }
    // 清理flush状态的缓冲区
    while (!flushbufqueue.empty())
    {
        LogBuffer *p = flushbufqueue.front();
        flushbufqueue.pop();
        delete p;
    }
    std::cout << "Logger::~Logger 函数结束" << std::endl;
}

/*
 * 日志类初始化函数
 *
 */
void Logger::Init(const char *logdir)
{
    // currentlogbuffer = new LogBuffer(BUFSIZE);
    buftotalnum++;
    time_t t = time(nullptr);       // 现在时刻
    struct tm *ptm = localtime(&t); // 类型转换
    char logfilepath[256] = {0};    // 日志文件名
    snprintf(logfilepath, 255, "%s/log_%d_%d_%d", logdir, ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
    if (!(fp = fopen(logfilepath, "w+")))
    {
        printf("Logger::Init 打开日志文件失败\n");
    }
    // 工作线程，将日志从缓冲区写入文件
    flushthread = std::thread(&Logger::Flush, this);
}

/*
 * 日志类添加数据到缓冲区
 *
 */
void Logger::Append(int level, const char *file, int line, const char *func, const char *fmt, ...)
{    
    // std::cout << "Logger::Append 函数触发" << std::endl;
    char logline[LOGLINESIZE]; // 单行日志内容
    struct timeval tv;
    gettimeofday(&tv, NULL);
    static time_t lastsec = 0;
    // 秒数不变则不调用localtime且继续复用之前的年月日时分秒的字符串，减少snprintf中太多参数格式化的开销
    if (lastsec != tv.tv_sec)
    {
        struct tm *ptm = localtime(&tv.tv_sec);
        lastsec = tv.tv_sec;
        int k = snprintf(save_ymdhms, 64, "%04d-%02d-%02d %02d:%02d:%02d", ptm->tm_year + 1900,
                         ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
        save_ymdhms[k] = '\0';
    }
    std::thread::id tid = std::this_thread::get_id(); // 当前线程id
    // uint32_t n = snprintf(logline, LOGLINESIZE, "[%s][%s.%03ld][%s:%d][%s][pid:%u] ", LevelString[level], save_ymdhms, tv.tv_usec / 1000, file, line, func, std::hash<std::thread::id>()(tid));
    uint32_t n = snprintf(logline, LOGLINESIZE, "[%s][%s.%03ld][%s:%d][%s] ", LevelString[level], save_ymdhms, tv.tv_usec / 1000, file, line, func);
    // uint32_t n = snprintf(logline, LOGLINESIZE, "[%s][%s.%03ld][%s:%d %s][] ", LevelString[level], save_ymdhms, tv.tv_usec/1000, file, line, func);
    // uint32_t n = snprintf(logline, LOGLINESIZE, "[%s][%04d-%02d-%02d %02d:%02d:%02d.%03ld]%s:%d(%s): ", LevelString[level], ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, tv.tv_usec/1000, file, line, func);
    va_list args;                                               // 额外参数
    va_start(args, fmt);                                        // 转化额外参数
    int m = vsnprintf(logline + n, LOGLINESIZE - n, fmt, args); // 将额外参数按照格式fmt输出到logline+n的位置，输出长度最大为LOGLINESIZE - n
    va_end(args);                                               // 释放可变参数资源
    int len = n + m;
    
    std::cout << "Logger::Append " << std::string(logline);

}

/*
 * 日志类写入数据到文件，线程回调函数
 *
 */
void Logger::Flush()
{
    std::cout << "Logger::Flush 函数触发" << std::endl;
    start = true;
    LogBuffer *p;
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(flushmtx);
            while (flushbufqueue.empty() && start)
            {
                flushcond.wait(lock);
            }
            std::cout << "Logger::Flush 信号量触发，准备工作" << std::endl;
            // 日志关闭，队列为空
            if (flushbufqueue.empty() && start == false)
            {
                std::cout << "Logger::Flush 工作线程关闭" << std::endl;
                return;
            }
            // 取出flushbufqueue的队头缓冲区并写入到文件
            p = flushbufqueue.front();
            flushbufqueue.pop();
        }
        p->FlushToFile(fp);
        p->SetState(LogBuffer::BufState::FREE);
        {
            std::lock_guard<std::mutex> lock(freemtx);
            freebufqueue.push(p);
        }
    }
}
