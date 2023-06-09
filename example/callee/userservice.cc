#include <iostream>
#include <string>
#include "jhrpcapplication.h"
#include "rpcprovider.h"
#include "user.pb.h"

// 先写的这段，然后从这段业务倒推我们的框架该如何设计，要使用就必须继承protobuf生产的对应类，然后重写其函数
// 这是框架构建的一步步过程，需要理解清楚

// rpc服务提供者  直接继承并重写
class UserService : public us::UserServiceRpc
{
public:
    bool Login(std::string &name, std::string &pwd)
    {
        // 打印表示业务
        std::cout << "doing local service : Login" << std::endl;
        std::cout << "name:" << name << "pwd" << pwd << std::endl;

        return true;
    }
    bool Register(uint32_t id, std::string &name, std::string &pwd)
    {
        if (id == 199 && name == "ljh" && pwd == "12345")
        {
            std::cout << "thats right" << std::endl;
            return true;
        }
        return false;
    }
    // 重写基类UserServiceRpc的虚函数  这些方法应该由框架帮我们匹配（我们在用框架中）
    //
    void Login(google::protobuf::RpcController *controller,
               const ::us::LoginRequest *request,
               ::us::LoginResponse *response,
               ::google::protobuf::Closure *done) // 最后一个参数执行回调，把返回值和其它信息返回
    {
        // 反序列化直接由protobuf完成
        // 框架给业务上报了请求参数LoginRequest，业务获取相应数据作本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        //...本地业务逻辑
        bool login_res = Login(name, pwd);

        // 写回响应
        us::ReturnCode *rc = response->mutable_result();
        rc->set_errcode(0);
        rc->set_errdesc("");
        response->set_sucess(login_res);

        // 执行回调  可以深入Closure再看
        // 纯虚函数，直接用一个匿名函数对象    执行响应消息的序列化以及网络发送 （由框架完成）
        done->Run();
    }

    void Register(google::protobuf::RpcController *controller,
                  const ::us::RegisterRequest *request,
                  ::us::RegisterResponse *response,
                  ::google::protobuf::Closure *done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool res = Register(id,name,pwd);

        us::ReturnCode* rc2 = response->mutable_result();
        if(res)
        {
            rc2->set_errcode(0);
            rc2->set_errdesc("");
        }
        else
        {
            rc2->set_errcode(1);
            rc2->set_errdesc("something error");
        }
        response->set_sucess(res);
        

        done->Run();
    }
};




int main(int argc, char *argv[])
{
    // 思考如何用这个框架 -- 让别人用起来简单，这也是我们需要做得，项目难点

    // 初始化
    JhrpcApplication::Init(argc, argv);

    // 把刚才的服务对象发布到rpc节点上
    RpcProvider provider; // 使用者使用时把它当作一个rpc节点
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点  Run以后，进程进入阻塞状态，等待远程Rpc调用请求
    provider.Run();
}
