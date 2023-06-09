#pragma once
//框架读取配置文件类
#include <unordered_map>
#include <string>

//rpcserverip port   zookeeperip port

class JhrpcConfig
{
public:
    void LoadConfigFile(const char*);
    //查询
    std::string Load(const std::string&);
private:
    std::unordered_map<std::string,std::string> configMap;
};