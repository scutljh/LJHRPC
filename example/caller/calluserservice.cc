#include <iostream>
#include "jhrpcapplication.h"
#include "user.pb.h"
#include "jhRpcChannel.h"
//模拟rpc调用方的行为

int main(int argc,char* argv[])
{
    JhrpcApplication::Init(argc,argv);

    //通过代理对象stub来调用
    us::UserServiceRpc_Stub stub(new JhrpcChannel());  //传入channel对象

    //理解channel的行为，channel和网络无关，只是一种中继
    

    //请求参数肯定由发送方填
    // us::LoginRequest request;
    // request.set_name("ljh");
    // request.set_pwd("123456");

    // //响应 直接不管
    // us::LoginResponse response;


    // //请求方不管closure
    // //同步调用过程
    // stub.Login(nullptr,&request,&response,nullptr); //底层统一是channel去调用callmethod

    // //一次rpc调用完成  response被填入了
    // if(response.result().errcode()!=0)
    // {
    //     std::cout<<"rpc login response error: "<<response.result().errdesc()<<std::endl;
    // }

    // std::cout<<"rpc login response success: "<<response.sucess()<<std::endl;


    //测试新的函数也能使用，决定包装一下，让用户只看得到自己使用的方法内容
    us::RegisterRequest regrequest;
    regrequest.set_id(199);
    regrequest.set_name("ljh");
    regrequest.set_pwd("123456");

    us::RegisterResponse regresponse;

    stub.Register(nullptr,&regrequest,&regresponse,nullptr);

    if(regresponse.sucess())
    {
        std::cout<<"success!"<<std::endl;
    }
    else
    {
        std::cout<<"failed to register: "<<regresponse.result().errdesc()<<std::endl;
    }

    return 0;
}