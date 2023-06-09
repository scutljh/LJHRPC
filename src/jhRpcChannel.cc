#include "jhRpcChannel.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <logger.h>
#include "zookeeperutil.h"

// 协议：请求在网络中的格式  4字节header长+header(+arg_len)+arg_msg
//   具体细分: headers_len + service_name method_name arg_len + arg_msg
// channel核心
void JhrpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller, const google::protobuf::Message *request,
                              google::protobuf::Message *response, google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *serviceDesc_p = method->service();
    const std::string &service_name = serviceDesc_p->name();
    const std::string &method_name = method->name();

    // 不要忘记protobuf的基本使用 怎么获取参数
    std::string args_str;
    if (!request->SerializeToString(&args_str))
    {
        // 替换为此ctller
        controller->SetFailed("failed to serialize");
        LOG_FATAL("failed to serialize");
        return;
    }
    uint32_t args_len = args_str.size();

    // 拼接网络中的发送格式
    // const std::string send_str = std::to_string(headers_len)+service_name+method_name+std::to_string(args_len)+args_str;
    // 忘记了，这里可以直接使用之前定义的IDL  RpcHeaders  把三个直接写在一起
    jhrpc::RpcHeader rpch;
    rpch.set_service_name(service_name);
    rpch.set_method_name(method_name);
    rpch.set_args_len(args_len);

    std::string headers;
    if (!rpch.SerializeToString(&headers))
    {
        controller->SetFailed("failed to serialize headers");
        LOG_FATAL("failed to serialize headers");
        return;
    }
    uint32_t headers_len = headers.size();
    // const std::string send_str = std::to_string(headers_len) + headers + args_str;
    std::string send_str;
    // 注意4字节的headers长，不多不少   用headers_len构造4字节的字符串
    send_str.insert(0, std::string((char *)&headers_len, 4));
    send_str += headers;
    send_str += args_str;

    // for debug
    // std::cout << "==================================" << std::endl;
    // std::cout << "header_size:" << headers_len << std::endl;

    // std::cout << "service_name:" << service_name << std::endl;
    // std::cout << "method_name:" << method_name << std::endl;
    // std::cout << "args_str:" << args_str << std::endl;
    // std::cout << "==================================" << std::endl;

    // 发送到网络之中  作为客户端，简单的tcp套接字即可，不考虑高并发
    // std::string ip = JhrpcApplication::GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(JhrpcApplication::GetConfig().Load("rpcserverport").c_str());

    // 分布式环境中，服务请求方本身是不能直接知道服务提供方的ip和端口号的  之前只是为了测试框架整体功能
    ZKClient zkCli;
    zkCli.start();
    std::string tofind_method_path = "/" + service_name + "/" + method_name;
    std::string host = zkCli.getData(tofind_method_path.c_str());
    if (host == "")
    {
        controller->SetFailed("there is no tofind_method_path " + tofind_method_path);
        LOG_FATAL("there is no tofind_method_path:%s\n ", tofind_method_path);
        return;
    }
    int sep_pos = host.find(":");
    if (sep_pos == std::string::npos)
    {
        controller->SetFailed("bad host:" + host);
        LOG_FATAL("bad host:%s", host);
        return;
    }

    std::string ip = host.substr(0, sep_pos);
    uint16_t port = atoi(host.substr(sep_pos + 1).c_str());

    // 发送到网络之中  作为客户端，简单的tcp套接字即可，不考虑高并发
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "failed to create socket errno: %d\n", errno);
        controller->SetFailed(errtxt);
        LOG_FATAL(errtxt);
        return;
    }
    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = inet_addr(ip.c_str());

    // 直连
    if (-1 == connect(clientfd, (sockaddr *)(&addr_in), sizeof(addr_in)))
    {
        controller->SetFailed("failed to connect");
        LOG_FATAL("failed to connect");
        close(clientfd);
        return;
    }

    // rpc请求发送
    if (-1 == send(clientfd, send_str.c_str(), send_str.size(), 0))
    {
        controller->SetFailed("failed to send");
        LOG_FATAL("failed to send");
        close(clientfd);
        return;
    }

    // 处理响应
    char rcv_buffer[512] = {0};
    int rcv_buffer_len = 0;
    if (-1 == (rcv_buffer_len = recv(clientfd, rcv_buffer, 512, 0)))
    {
        controller->SetFailed("failed to recv");
        LOG_FATAL("failed to recv");
        close(clientfd);
        return;
    }
    close(clientfd);

    // 写回response
    // p1. char*数组中\0后的字符存不下，string不完整，反序列化也失败
    // std::string response_str(rcv_buffer,rcv_buffer_len);

    // 看架构图 这里需要作反序列化，我们是caller端的stub部分
    if (!response->ParseFromArray(rcv_buffer, rcv_buffer_len))
    {
        controller->SetFailed("failed to parse from string");
        LOG_FATAL("failed to parse from string");
        return;
    }
}
