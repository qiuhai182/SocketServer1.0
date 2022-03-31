#pragma once

#ifndef _TIMETOOL_H_
#define _TIMETOLL_H_

#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <cstring>
#include <list>
#include <functional>
#include <thread>
#include <mutex>

const int maxLength = 1024; // 某些数据的最大长度

using namespace std;

namespace Times
{

    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(char *timeString)
    {
        int iY = atoi(timeString); // atoi函数遇到非数字自动结束取位
        int iM = atoi(timeString + 5);
        int iD = atoi(timeString + 8);
        int iH = atoi(timeString + 11);
        int iMin = atoi(timeString + 14);
        int iS = atoi(timeString + 17);
        struct tm timeInfo;
        memset(&timeInfo, 0, sizeof(timeInfo));
        timeInfo.tm_year = iY - 1900;
        timeInfo.tm_mon = iM - 1;
        timeInfo.tm_mday = iD;
        timeInfo.tm_hour = iH;
        timeInfo.tm_min = iMin;
        timeInfo.tm_sec = iS;
        return mktime(&timeInfo);
    }

    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(const char *timeString)
    {
        return get_timeStamp(const_cast<char *>(timeString));
    }

    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(const string &timeString)
    {
        char timeChar[maxLength];
        for (int i = 0; i < timeString.size(); ++i)
        {
            timeChar[i] = timeString.data()[i];
        }
        return get_timeStamp(timeChar);
    }

    /*
    * 根据时间格式：xxxx-xx-xx xx:xx:xx
    * 得到int时间戳
    */
    long get_timeStamp(string &timeString)
    {
        return get_timeStamp(timeString.data());
    }

    /*
    * 判断收到的时间是否是日期格式
    * 是则返回True
    * 若是时间戳则返回False
    */
    bool is_stampTime(string recvTime)
    {
        string id = recvTime.substr(4, 1);
        return "-" == id;
    }

    /*
    * 根据int时间戳
    * 得到string时间
    */
    string get_stampTime(int timeStamp)
    {
        char res[1024];
        time_t tick = (time_t)timeStamp;
        struct tm timeInfo = *localtime(&tick);
        strftime(res, sizeof(res), "%Y-%m-%d %H:%M:%S", (struct tm *)&timeInfo);
        return res;
    }
    
    /*
     * 得到此刻时间：xxxx-xx-xx
     * 
     */
    string get_nowTime()
    {
        time_t timeValue = 0;
        time(&timeValue);
        struct tm *curTime = gmtime(&timeValue);
        int year = curTime->tm_year + 1900;
        int month = curTime->tm_mon + 1;
        int day = curTime->tm_mday;
        int weekenday = curTime->tm_wday;
        int hour = curTime->tm_hour;
        int minute = curTime->tm_min;
        int second = curTime->tm_sec;
        string preMonth = month > 9 ? "" : "0";
        string preDay = day > 9 ? "" : "0";
        return string("" + to_string(year) + "-" + preMonth + to_string(month) + "-" + preDay + to_string(day));
    }

    /*
     * 得到此刻时间戳
     * 
     */
    int get_nowTimeStamp()
    {
        time_t timeValue = 0;
        time(&timeValue);
        struct tm *curTime = gmtime(&timeValue);
        int year = curTime->tm_year + 1900;
        int month = curTime->tm_mon + 1;
        int day = curTime->tm_mday;
        int weekenday = curTime->tm_wday;
        int hour = curTime->tm_hour;
        int minute = curTime->tm_min;
        int second = curTime->tm_sec;
        string preMonth = month > 9 ? "" : "0";
        string preDay = day > 9 ? "" : "0";
        string preHour = hour > 9 ? "" : "0";
        string preMinute = minute > 9 ? "" : "0";
        string preSecond = second > 9 ? "" : "0";
        string timeString = to_string(year) + "-" + preMonth + to_string(month) + "-" + preDay + 
                            to_string(day) + " " + preHour + to_string(hour) + "-" + preMinute + 
                            to_string(minute) + "-" + preSecond + to_string(second);
        return get_timeStamp(timeString);
    }

}
     
#endif