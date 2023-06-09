#include "logger.h"
#include <iostream>


Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    std::thread wrlog2disk([&](){
        while(1)
        {
            //从队列中拿日志信息，写入日志文件中 （天为单位）
            time_t now = time(0);
            tm* nowtm = localtime(&now);

            char file_name[64];
            sprintf(file_name,"%d-%d-%d-log.txt",nowtm->tm_year+1900,nowtm->tm_mon+1,nowtm->tm_mday);

            FILE* fop = fopen(file_name,"a+");
            if(fop == nullptr)
            {
                std::cout<<"log file "<<file_name<<" cant open "<<std::endl;
                exit(EXIT_FAILURE);
            }
            //从队列拿
            std::string msg = logQ.PopLog();
            char time_buf[64] = {0};
            sprintf(time_buf,"[%s][%d:%d:%d]",(logLevel==INFO?"INFO":"FATAL"),nowtm->tm_year+1900,nowtm->tm_mon+1,nowtm->tm_mday);
            msg.insert(0,time_buf);
            msg.append("\n");

            fputs(msg.c_str(),fop);
            fclose(fop);
        }
    });
    wrlog2disk.detach();
}

void Logger::SetLogLevel(Loglevel level)
{
    logLevel = level;
}

//写到log queue中
void Logger::Log(std::string msg)
{
    logQ.PushLog(msg);
}