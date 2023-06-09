#include "jhrpcapplication.h"
#include "friend.pb.h"


//用户的请求端使用
int main(int argc,char* argv[])
{
    JhrpcApplication::Init(argc,argv);

    frd::friendServiceRpc_Stub stub(new JhrpcChannel());
    
    frd::GetFriendListsRequest frdreq;
    frd::GetFriendListsResponse frdresp;
    frdreq.set_userid(12333);
    //调用过程中的一些状态信息可用此获取
    jhrpcController jhctl;
    stub.GetFriendLists(&jhctl,&frdreq,&frdresp,nullptr);

    if(jhctl.Failed())
    {
        std::cout<<jhctl.ErrorText()<<std::endl;
        return 0;
    }

    if(frdresp.success())
    {
        std::cout<<"success to get list!"<<std::endl;
        for(int i=0;i<frdresp.friends_size();++i)
        {
            std::cout<<frdresp.friends(i)<<std::endl;
        }
    }
    else
    {
        std::cout<<"cannot get friend list for "<<frdreq.userid()<<std::endl;
    }
    return 0;
}
