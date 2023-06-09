#pragma once
#include "google/protobuf/service.h"
#include <functional>
#include <google/protobuf/descriptor.h>
#include <string>
#include <unordered_map>

#include "../netlayer/TcpServer.h"
#include "../netlayer/EventLoop.h"
#include "../netlayer/InetAddress.h"
#include "../netlayer/TcpConnection.h"
#include "../netlayer/TimeStamp.h"

// 框架需要泛用性  回归最开始的protobuf对函数是如何包装的，发现是继承下来的

// 框架提供的rpc服务提供者   专用于发布rpc服务到节点上
// 初期为了实现简单，我先使用muduo库，其实就是一个muduo的简易server
class RpcProvider
{
public:
    // 传入参数回归protobuf底层原理
    void NotifyService(google::protobuf::Service *); // 向服务节点中添加并发布函数的服务接口

    // 启动rpc服务节点，开始提供rpc服务
    void Run();

private:
    // 服务提供者必须维护服务类型和服务对应的一张表，才能对到来的rpc请求做出相关响应
    struct ServiceInfo
    {
        // 保存服务对象
        google::protobuf::Service *m_service;
        // 服务对象里有哪些方法  方法名字对应方法描述指针
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> method_table;
    };

    // 多服务类型
    // 存储了注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> ServiceTypeMap;

    // 组合了EventLoop  必须使用的
    EventLoop mn_eventLoop;

    // socket连接回调
    void ConnectionCB(const TcpConnectionPtr &);
    // socket消息读写回调
    void MessageRWCB(const TcpConnectionPtr &, Buffer *, TimeStamp);


    // Closure回调操作，将响应序列化以后并发送到网络之中  -- 还是和protobuf强相关
    void SendRpcResponse(const TcpConnectionPtr &, google::protobuf::Message *);
};