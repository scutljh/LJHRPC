#pragma once

//让eventloop高效地获取当前线程id
namespace CurrentThread
{
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        //此语法允许程序员将最有可能执行的分支告诉编译器  
        //__builtin_expect(EXP, N)。意思是：EXP==N的概率很大
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}