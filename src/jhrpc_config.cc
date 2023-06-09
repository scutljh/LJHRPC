#include "jhrpc_config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "logger.h"

void JhrpcConfig::LoadConfigFile(const char * config_file)  //后续可再修补，不是关键
{
    std::ifstream file(config_file);
    if(file.fail()==true)
    {
        LOG_FATAL("failed to load config file");
        exit(EXIT_FAILURE);
    }

    //读取FILE
    std::string line;
    while(std::getline(file,line))
    {
        if(line[0]=='#')
        {
            continue;
        }
        std::istringstream linestream(line); // 将当前行字符串转换为字符流
        std::string label,value;
        if(std::getline(linestream,label,'=')&&std::getline(linestream, value))
        {
            configMap[label]=value;
        }
    }
}

std::string JhrpcConfig::Load(const std::string & key)
{
    if(configMap.count(key))
    {
        return configMap[key];
    }
    return "";
}