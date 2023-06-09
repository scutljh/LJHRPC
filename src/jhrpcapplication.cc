#include "./include/jhrpcapplication.h"
#include <iostream>
#include <string>
#include <unistd.h>

JhrpcApplication *JhrpcApplication::_jh = nullptr;
mutex JhrpcApplication::_mtx;
JhrpcConfig JhrpcApplication::_jhconfig;


void ShowArgsHelp()
{
    std::cout << "Usage: command -i <configfile>" << std::endl;
}

void JhrpcApplication::Init(int argc, char *argv[])
{
    if (argc < 2)
    {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    // 使用getopt获取命令行行参数
    int c = 0;
    std::string config_file;
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    // 加载配置文件
    JhrpcApplication::_jhconfig.LoadConfigFile(config_file.c_str());
    
}

JhrpcApplication &JhrpcApplication::GetInstence()
{
    if (JhrpcApplication::_jh == nullptr)
    {
        JhrpcApplication::_mtx.lock();
        if (JhrpcApplication::_jh == nullptr)
        {
            JhrpcApplication::_jh = new JhrpcApplication();
        }
        JhrpcApplication::_mtx.unlock();
    }
    return *JhrpcApplication::_jh;

    // static JhrpcApplication jhapp;
    // return jhapp;
}

JhrpcConfig& JhrpcApplication::GetConfig()
{
    return _jhconfig;
}