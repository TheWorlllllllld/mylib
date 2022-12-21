#pragma once

#include <string>
#include <iostream>

#include "noncopyable.h"

#define LOG_INFO(LogmsgFormat, ...)\
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = { 0 }; \
        snprintf(buf,1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) //结束do-while
// printf("hellw %a", 1.2);
#define LOG_ERROR(LogmsgFormat, ...)\
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = { 0 }; \
        snprintf(buf,1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) //结束do-while

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...)\
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = { 0 }; \
        snprintf(buf,1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) //结束do-while
#else
    #define LOG_DEBUG(LogmsgFormat, ...)
#endif


#define LOG_FATAL(LogmsgFormat, ...)\
    do \
    { \
        Logger &logger = Logger::Instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = { 0 }; \
        snprintf(buf,1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(1); \
    }while(0) //结束do-while

//定义日志的级别 从低到高
enum LogLevel
{
    DEBUG,//调试信息
    INFO,//普通信息
    ERROR,//错误信息
    FATAL//崩溃了
};

//日志类
class Logger : noncopyable
{
public:
    //单例
    static Logger& Instance();
    //设置级别
    void setLogLevel(LogLevel level);
    //写日志
    void log(std::string msg);
private:
    Logger() = default;
    ~Logger() = default;
    int logLevel_;//日志级别
};