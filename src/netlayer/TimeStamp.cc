#include "TimeStamp.h"
#include <ctime>

TimeStamp::TimeStamp()
    : microSeconds_(0)
{
}
TimeStamp::TimeStamp(int64_t microSeconds)
    : microSeconds_(microSeconds)
{
}

TimeStamp TimeStamp::now()
{
    return TimeStamp(time(NULL));
}
std::string TimeStamp::TstoString() const
{
    char timebuf[128] = {0};

    //使用localtime获取日期时间
    tm* tm_time = localtime(&microSeconds_);

    snprintf(timebuf,128,"%4d/%02d/%02d %02d:%02d:%02d",tm_time->tm_year+1900,
        tm_time->tm_mon+1,tm_time->tm_mday,tm_time->tm_hour,tm_time->tm_min,tm_time->tm_sec);
    return timebuf;
}