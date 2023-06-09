#pragma once
#include <jhrpcapplication.h>
#include <google/protobuf/service.h>
#include <rpcheader.pb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../netlayer/TcpServer.h"
#include "../netlayer/EventLoop.h"
#include "../netlayer/InetAddress.h"
#include "../netlayer/TcpConnection.h"


//所有rpc服务请求者（stub代理对象）殊途同归，最后来到rpcchannel调用的callmethod方法中
class JhrpcChannel : public google::protobuf::RpcChannel
{

public:
    //主要做序列化和网络发送  rpc请求调用核心
    virtual void CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller, const google::protobuf::Message *request,
                            google::protobuf::Message *response, google::protobuf::Closure *done);
};