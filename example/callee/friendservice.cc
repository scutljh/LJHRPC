#include "jhrpcapplication.h"
#include "friend.pb.h"
#include "rpcprovider.h"
#include <vector>
#include <string>
#include "logger.h"

//用户的服务端使用
//感觉成本还是有点高？  后续再封装一层 或自动生成代码
class friendService : public frd::friendServiceRpc
{
public:
    virtual void GetFriendLists(google::protobuf::RpcController *controller,
                                const ::frd::GetFriendListsRequest *request,
                                ::frd::GetFriendListsResponse *response,
                                ::google::protobuf::Closure *done)
    {
        uint32_t userID = request->userid();
    
        std::vector<std::string> list;
        bool ret = GetFriendLists(userID, list);
        response->set_success(ret);

        // protobuf用的不熟，repeated类型默认size多少？
        // 其实根本不用管size，add_friends就是让你添加一次而已
        if (ret)
        {
            for (auto &str : list)
            {
                std::string *one = response->add_friends();
                *one = str;
            }
        }
        

        done->Run();
    }

private:
    // 用户在自己的服务端写好
    bool GetFriendLists(uint32_t userId, std::vector<std::string> &vs)
    {
        // 本地业务
        if (userId==31866)
        {
            vs.assign({"ljh", "dsad", "bobo"});
            return true;
        }
        else if (userId== 12333)
        {
            vs.assign({"alice", "momo", "nafei"});
            return true;
        }
        else
        {
            return false;
        }
    }
};

int main(int argc, char *argv[])
{
    LOG_INFO("hello log!");


    JhrpcApplication::Init(argc, argv);

    RpcProvider rpcProv;
    rpcProv.NotifyService(new friendService());

    rpcProv.Run();
}