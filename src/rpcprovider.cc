#include "./include/rpcprovider.h"
#include "jhrpcapplication.h"
#include "logger.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"

// 服务提供者发布服务到节点上
// 用户如何注册，我们如何存储用户要发布的方法？  远程来调用，我们从这张map表中找
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;
    // 拿到服务对象的描述信息，这个由protobuf生成
    const google::protobuf::ServiceDescriptor *serviceDesc_p = service->GetDescriptor();

    // 获取服务名
    const std::string service_name = serviceDesc_p->name();
    int MethodCount = serviceDesc_p->method_count();

    // 拿方法并存储
    for (int i = 0; i < MethodCount; ++i)
    {
        const google::protobuf::MethodDescriptor *methodDesc_p = serviceDesc_p->method(i);
        std::string method_name = methodDesc_p->name();
        service_info.method_table.insert({method_name, methodDesc_p});
    }
    service_info.m_service = service;
    ServiceTypeMap.insert({service_name, service_info});
}

// rpc服务提供节点
void RpcProvider::Run()
{
    std::string ip = JhrpcApplication::GetConfig().Load("rpcserverip");
    uint16_t port = atoi(JhrpcApplication::GetConfig().Load("rpcserverport").c_str());

    // 以下连接步骤使用muduo网络库完成，后续可能会被替换为自实现
    // 目前使用muduo库仅仅是因为其能够很好地分离网络和业务

    // 创建tcpserver对象
    InetAddress address(port,ip);
    TcpServer server(&mn_eventLoop, address, "RpcProvider");

    // 使用bind绑定器，因为这里要传入的是类对象的方法，得包装一下（c面向过程方法直接传名字即可）
    // 处理连接回调
    server.setConnectionCallback(std::bind(&RpcProvider::ConnectionCB, this, std::placeholders::_1));
    // 处理消息读写回调
    server.setMessageCallback(std::bind(&RpcProvider::MessageRWCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置muduo库的线程数量  //1个io 3个工作
    server.setThreadNum(4);

    // 将此rpc节点上的已经发布的服务全部注册到zk中，便于rpc请求者调用服务
    ZKClient zkCli;
    zkCli.start();
    // 所有的服务节点添加为永久，方法节点为临时
    for (auto &servop : ServiceTypeMap)
    {
        std::string service_path = "/"+servop.first;
        zkCli.createNode(service_path.c_str(),nullptr,0,0);

        for(auto& methop:servop.second.method_table)
        {
            std::string method_path = service_path+"/"+methop.first;
            //存的数据就是当前该主机的ip和端口，存到zk的server上
            char _data[128]={0};
            sprintf(_data,"%s:%d",ip.c_str(),port);
            //临时性节点
            zkCli.createNode(method_path.c_str(),_data,strlen(_data),ZOO_EPHEMERAL);
        }
    }

    LOG_INFO("RpcProvider start service at ip:%d port:%d\n", ip, port);

    // 启动网络服务
    server.start();
    mn_eventLoop.loop();
}

// call back func
void RpcProvider::ConnectionCB(const TcpConnectionPtr &conn)
{
    // 和rpc_client的链接断开了
    if (!conn->connected())
    {
        conn->shutdown();
    }
}

/*
在框架内部，Rpcprovider和RpcConsumer之间需要协商好通信的protobuf数据类型 （IDL本身价值）
如：service_name method_name args   可额外定义proto的message类型进行数据头序列化和反序列化

16UserServiceLogin...
header_size(4字节) + header_str(+args_len)+args_str   为了防止粘包，需要再定义args len也放在headers中

长度实实在在地存成二进制，占用字符串固定前4个字节，便于我们解包
*/

void RpcProvider::MessageRWCB(const TcpConnectionPtr &conn, Buffer *buffer, TimeStamp tsp)
{
    // 远程请求字符流全部转化为字符串先
    std::string recv_buf = buffer->retrieveAllAsString();

    // 格式如何组织?
    // 需要自定义一个协议  -- 又一个比较麻烦的点

    // 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    // 从0开始读，拷贝4个字节的内容放到第一个参数的位置上
    recv_buf.copy((char *)&header_size, 4, 0);

    // 据此size读取数据头的原始字符流  然后反序列化为IDL
    std::string rpc_header_str = recv_buf.substr(4, header_size);

    jhrpc::RpcHeader rpcHeader;
    if (!rpcHeader.ParseFromString(rpc_header_str))
    {
        // 后期打日志
        // 数据头反序列化失败
        LOG_FATAL("rpc rcvd header parse error");
        return;
    }
    std::string service_name = rpcHeader.service_name();
    std::string method_name = rpcHeader.method_name();
    uint32_t args_len = rpcHeader.args_len();

    // 获取rpc方法参数的字符流
    std::string args_str = recv_buf.substr(4 + header_size, args_len);

    // for debug

    // 获取服务对象  方法对象
    auto sit = ServiceTypeMap.find(service_name);
    if (sit == ServiceTypeMap.end())
    {
        LOG_FATAL("service %s doce not exists", service_name);
        return;
    }

    auto mit = sit->second.method_table.find(method_name);
    if (mit == sit->second.method_table.end())
    {
        LOG_FATAL("method %s doce not exists", method_name);
        return;
    }

    // 获取到了
    google::protobuf::Service *service = sit->second.m_service; // 此处可理解为对于example中用户发布的那个对象，任意
    const google::protobuf::MethodDescriptor *methodDesc = mit->second;

    // 生成rpc调用的请求request 和 响应response参数  并填入  符合protobuf中的调用
    // GetRequestPrototype返回一个Message对象，其实就是我们服务对象的方法的请求参数类型，拿出这个结构体，然后我们填入
    google::protobuf::Message *request = service->GetRequestPrototype(methodDesc).New(); // 此调用很关键
    if (!request->ParseFromString(args_str))
    {
        LOG_FATAL("request parse error");
        return;
    }

    google::protobuf::Message *response = service->GetResponsePrototype(methodDesc).New();

    // 这里是下面写完加上的，和Callmethod的最后一个参数有关，Closure，负责请求完成以后的回调
    // 绑定一个回调函数(protobuf提供的) -- NewCallback  这个东西返回一个Closure的指针
    // NewCallback有很多版本，我的是成员函数带两个参数，选择什么自然非常清晰
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                                                    const TcpConnectionPtr &,
                                                                    google::protobuf::Message *>(this, &RpcProvider::SendRpcResponse, conn, response);

    // newcallback就帮我们做了重写closure中run方法的事

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法

    // 最开始我们写的用户的UserService().Login(controller,request,response,done)，其实就是:
    service->CallMethod(methodDesc, nullptr, request, response, done);
    // 再次体现了我们的框架的抽象与protobuf给我们生成的继承链强相关，仔细查看service中的callmethod
}

// 将结果响应回网络，那显然我们需要网络和响应内容
void RpcProvider::SendRpcResponse(const TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;
    if (!response->SerializeToString(&response_str))
    {
        LOG_FATAL("serialize failed! ");
        return;
    }
    conn->send(response_str);
    conn->shutdown(); // 短链接，服务提供方完成服务主动断开链接，来给更多的请求者提供服务
}