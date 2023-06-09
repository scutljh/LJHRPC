#include "test.pb.h"
#include <iostream>
#include <fstream>
using namespace std;

int main()
{
    // string send_str;
    // //序列化
    // {
    //     mytest::LoginRequest req;
    //     req.set_name("ljh");
    //     req.set_pswd("123456");

    //     // fstream input("test.bin",ios::in|ios::binary);
    //     // if(req.SerializeToOstream(&input))

    //     if (req.SerializeToString(&send_str))
    //     {
    //         cout << send_str.c_str() << endl;
    //     }
    // }

    // // 反序列化
    // {
    //     mytest::LoginRequest req;
    //     if (req.ParseFromString(send_str))
    //     {
    //         cout<<req.name()<<" "<<req.pswd()<<endl;
    //     }
    // }

    mytest::LoginResponse resp;
    mytest::ResultCode* rc = resp.mutable_res();

    rc->set_errnum(1);
    rc->set_errstr("failed to login");


    mytest::GetFriendListResPonse gflresp;
    //相关get方法获取数组
    mytest::User* users = gflresp.add_friend_list();

    users->set_name("ljh");
    //.....


}
