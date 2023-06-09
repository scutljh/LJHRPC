# jhrpc

## 前言



>  本框架高度依赖protobuf实现，请确保你的计算机上安装并编译好了protobuf。
>
>  ----
>
>  目前还欠缺一些封装，因此本框架需要使用者至少掌握proto文件的编写，不过不用担心，protobuf本身足够强大，上手非常容易。
>
>  同时本框架的服务中心使用了开源的zookeeper，自行搜索安装配置

## 如何编译

> 直接运行根目录下的autobuild.sh脚本文件，rpc框架将会被编译为静态库，你所需要的头文件也都在lib目录下

## 使用示例

> 在example目录下，我写了一个非常简单的示例以帮助你上手框架
>
> ---
>
> 如果你不熟悉protobuf，那么最好将你想要设计为rpc方法的对应参数和返回值，以及方法名，按照以下格式书写

```protobuf
syntax = "proto3";

//如果你要传递方法，此option是必不可少的
option cc_generic_services = true;

package test;  

//你想要设计为rpc方法的方法参数
message AddRequest
{
    int32 lhs = 1 ;
    int32 rhs = 2 ;
}
//返回值
message AddResponse
{
    int32 result = 1;
}
//方法本身
service MathService  //请注意你的service命名，你将在你的server程序中继承同名类
{
    rpc Add(AddRequest) returns(AddResponse);
}
```

> 编译proto文件，并在你的用户端和客户端程序中加入此pb文件的头文件

> 请一定注意你在proto文件中定义的命名空间，它将在对应的c文件中体现

```c++
//你的服务端需要如下书写：
#include "jhrpcapplication.h"
#include "rpcprovider.h"
#include "user.pb.h"

// 继承你在proto文件中写的service同名类
class mathServe : public test::MathService
{
public:
    // 重写其中你添加的方法，可进入类中看
    virtual void Add(google::protobuf::RpcController *controller,
                     const ::test::AddRequest *request,
                     ::test::AddResponse *response,
                     ::google::protobuf::Closure *done)
    {
        // 将参数拿出
        uint32_t lnum = request->lhs();
        uint32_t rnum = request->rhs();

        // 调用你的本地方法
        uint32_t ret = Add(lnum, rnum);

        // 填入响应
        response->set_result(ret);
        // 回调执行后续流程
        done->Run();
    }

private:
    int Add(int l, int r)
    {
        return l + r;
    }
};

int main(int argc, char *argv[])
{
    JhrpcApplication::Init(argc, argv);

    RpcProvider rpcProv;
    rpcProv.NotifyService(new mathServe());

    rpcProv.Run();
}
```

```c++
//你的客户端需要如下书写：
int main(int argc,char* argv[])
{
    JhrpcApplication::Init(argc,argv);

    test::MathService_Stub stub(new JhrpcChannel());

    //填入你的参数
    test::AddRequest areq;
    areq.set_lhs(100);
    areq.set_rhs(200);

    test::AddResponse aresp;
    //调用方法处
    stub.Add(nullptr,&areq,&aresp,nullptr);

    std::cout<<aresp.result()<<std::endl;
}
```

> 同时项目目前还没有考虑生产环境中的问题,我会在后期对程序进行优化和进一步封装

