#include "zookeeperutil.h"
#include "logger.h"

// 链接zookeeper_mt多线程版本

//感觉简单的zk所有的都可以这么写

ZKClient::ZKClient()
    : zkhandler(nullptr)
{
}

ZKClient::~ZKClient()
{
    if (zkhandler != nullptr)
    {
        zookeeper_close(zkhandler);
    }
}
// zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    //回调事件发生了，就唤醒线程
    if(type == ZOO_SESSION_EVENT)  //会话相关
    {
        if(state == ZOO_CONNECTED_STATE)  //zkclient和zkserver链接成功
        {
            sem_t* sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);  //被这个信号量阻塞的线程在此信号量变为>=0以后会被唤醒
        }
    }
}

// zkclient启动连接zkserver
void ZKClient::start()
{
    std::string zkip = JhrpcApplication::GetConfig().Load("zookeeperip");
    std::string zkport = JhrpcApplication::GetConfig().Load("zookeeperport");
    std::string zkhost = zkip + ":" + zkport;

    /*
     * This method creates a new handle and a zookeeper session that corresponds
     * to that handle. Session establishment is asynchronous, meaning that the
     * session should not be considered established until (and unless) an
     * event of state ZOO_CONNECTED_STATE is received.
     * ------
     * 多线程版本的zk，api客户端程序提供了三个线程：
     * API调用线程
     * 网络I/O线程  底层用poll（毕竟客户端）
     * watcher回调线程
     */

    // 查看api要求的参数，自己对应着填入    超时时间设置为30000
    zkhandler = zookeeper_init(zkhost.c_str(), global_watcher, 30000, nullptr, nullptr, 0);

    if (zkhandler == nullptr)   //仅仅是句柄空间开辟
    {
        LOG_FATAL("zookeeper init failed !");
        exit(EXIT_FAILURE);
    }

    //zk服务不是调用了api就初始化成功的，回调被写入了才算初始化成功

    sem_t sem;
    sem_init(&sem,0,0);
    //给句柄资源设置上下文，把信号量添加进去   信号量由句柄控制的回调控来操作
    zoo_set_context(zkhandler,&sem);

    //初始化为0，p操作肯定会阻塞在此
    sem_wait(&sem);
    LOG_INFO("zookeeper init success !");
}
// state表示永久性/临时性节点  0为永久
void ZKClient::createNode(const char *node_path, const char *data, int data_sz, int state)
{
    char path_buffer[128]={0};
    int path_buffer_len = sizeof(path_buffer);
    int flag = zoo_exists(zkhandler,node_path,0,nullptr);

    if(flag == ZNONODE)
    {
        //真正在指定路径上创建一个znode节点   //这个api只关注前三个参数，后面的参数都是没办法的
        flag =zoo_create(zkhandler,node_path,data,data_sz,
            &ZOO_OPEN_ACL_UNSAFE,state,path_buffer,path_buffer_len);
        if(flag ==ZOK)
        {
            LOG_INFO("znode create success ! path:%s\n",node_path);
        }
        else
        {
            LOG_FATAL("flag:%d\n",flag);
            LOG_FATAL("znode create error ! path:%s\n",node_path);
            exit(EXIT_FAILURE);
        }
    }
}

std::string ZKClient::getData(const char *node_path)
{
    char buffer[64] = {0};
    int buffer_len = sizeof(buffer);
    int flag = zoo_get(zkhandler,node_path,0,buffer,&buffer_len,nullptr);

    if(flag!=ZOK)
    {
        LOG_INFO("get znode data error , path:%s\n",node_path);
        return "";
    }
    return buffer;
}