#pragma once
#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include "jhrpcapplication.h"
#include <string>

class ZKClient
{
public:
    ZKClient();
    ~ZKClient();

    //zkclient启动连接zkserver
    void start();
    //state表示永久性/临时性节点
    void createNode(const char* node_path,const char* data,int data_sz,int state);
    std::string getData(const char* node_path);
private:
    zhandle_t* zkhandler;   //zk中心的操作句柄
};
