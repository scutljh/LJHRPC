#pragma once
#include <iostream>

class TimeStamp
{
public:
    TimeStamp();
    explicit TimeStamp(int64_t microSeconds); //单参数构造函数以后习惯声明为explicit
    static TimeStamp now();
    std::string TstoString() const;
private:
    int64_t microSeconds_;
};