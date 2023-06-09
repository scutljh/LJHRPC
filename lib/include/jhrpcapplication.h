#pragma once
#include <mutex>
#include <iostream>
#include "jhrpc_config.h"
#include "jhRpcChannel.h"
#include "jhrpccontroller.h"

using std::mutex;
//整个框架我要构造为单例的对象


// 框架初始化类
class JhrpcApplication
{
public:
    //添加标记，如果已经初始化，则无需再初始化，后期完善
    static void Init(int argc,char* argv[]);

    static JhrpcApplication& GetInstence();
    static JhrpcConfig& GetConfig();
    

private:
    JhrpcApplication() {}
    JhrpcApplication(const JhrpcApplication&)=delete;
    JhrpcApplication(JhrpcApplication&&)=delete;
    JhrpcApplication& operator=(const JhrpcApplication&)=delete;

    static JhrpcApplication* _jh;
    static mutex _mtx;
    static JhrpcConfig _jhconfig;
};
