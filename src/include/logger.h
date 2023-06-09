#pragma once
#include "logqueue.h"
#include <string.h>
#include <thread>
#include <time.h>
#include <cstdio>

enum Loglevel
{
    INFO,   //提示信息
    FATAL //致命错误
};

class Logger
{
public:
    static Logger& GetInstance();

    void SetLogLevel(Loglevel level);

    void Log(std::string msg);
private:
    Logger();
    Logger(const Logger&)=delete;
    Logger(Logger&&)=delete;

    int logLevel;
    LogQueue<std::string> logQ;
};

#define LOG_INFO(logmsgformat,...) \
    do \
    {  \
        Logger& logger = Logger::GetInstance();\
        logger.SetLogLevel(INFO);\
        char str[1024] = {0};\
        snprintf(str,1024,logmsgformat,##__VA_ARGS__);\
        logger.Log(str);\
    } while(0);

#define LOG_FATAL(logmsgformat,...) \
    do \
    {  \
        Logger& logger = Logger::GetInstance();\
        logger.SetLogLevel(FATAL);\
        char str[1024] = {0};\
        snprintf(str,1024,logmsgformat,##__VA_ARGS__);\
        logger.Log(str);\
    } while(0);
