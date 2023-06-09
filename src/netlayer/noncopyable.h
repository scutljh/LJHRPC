#pragma once

//此工具类主要目的是让一些类无法被拷贝
class noncopyable
{
public:
    noncopyable(const noncopyable&)=delete;
    noncopyable& operator=(const noncopyable&)=delete;
protected:
    noncopyable()=default;
    ~noncopyable()=default;
};