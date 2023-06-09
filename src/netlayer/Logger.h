#pragma once
#include "noncopyable.h"

#include <string>
#include <stdlib.h>

enum LEVEL
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

//暴露给上层使用的简易宏
#define LOG_INFO(logmsgformat,...) \
    do \
    { \
        Logger& logger = Logger::GetInstance();\
        logger.setLogLevel(INFO);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
        logger.log(buf);\
    } while(0)

#define LOG_ERROR(logmsgformat,...)\
    do \
    { \
        Logger& logger = Logger::GetInstance();\
        logger.setLogLevel(ERROR);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
        logger.log(buf);\
    } while(0)

#define LOG_FATAL(logmsgformat,...)\
    do \
    { \
        Logger& logger = Logger::GetInstance();\
        logger.setLogLevel(FATAL);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
        logger.log(buf);\
        exit(-1);\
    } while(0)

#ifdef DEBUG__
#define LOG_DEBUG(logmsgformat,...)\
    do\
    {\
        Logger& logger = Logger::GetInstence();\
        logger.setLogLevel(DEBUG);\
        char buf[1024] = ={0};\
        snprintf(buf,1024,logmsgformat,##__VA_ARGS__);\
        logger.log(buf);\
    } while(0)
#else
    #define LOG_DEBUG(logmsgformat,...)
#endif


class Logger:noncopyable
{
public:
    static Logger& GetInstance();
    void setLogLevel(int level);
    void log(std::string msg);
private:
    int loglevel_;
    Logger()=default;
    Logger(const Logger&)=delete;
    void operator=(const Logger&)=delete;
};