#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <vector>       //用于存储多个线程
#include <thread>       //线程的创建及管理
#include <queue>        //用于存储多个任务的队列
#include <condition_variable>   //条件变量头文件
#include <mutex>        //互斥锁头文件
#include <functional>   //用于使用函数类型
#include <myhead.h>     //自定义头文件

#define N 128       //消息缓冲区的大小
#define LOGIN 1     //表示登录消息类型
#define CHAT 2      //表示聊天消息类型
#define QUIT 3      //表示退出消息类型

//自定义服务器类
class chatServer
{
public:
    //定义消息结构体
    struct MSG
    {
        int type;       //消息类型
        char name[20];   //客户端名称
        char text[N];   //消息内容

        //序列化函数：将消息结构体转为二进制数据，便于传输
        string serialize() const
        {
            string data;    //临时变量存储要转换的字符串
            data.append(reinterpret_cast<const char*>(&type),sizeof(type)); //将消息类型放入字符串中
            data.append(name,sizeof(name));    //将消息中的客户端名称放入字符串
            data.append(text,sizeof(text));     //将消息正文放入字符串

            return data;//将拼接好的字符串返回

        }

        //反序列化函数：将二进制数据转换为结构体
        void deseralize(const string &data)
        {
            size_t offset=0;    //截取字符串时的偏移量

            memcpy(&type,data.c_str()+offset,sizeof(type));//将字符串的消息类型解析出来
            offset+=sizeof(type);           //偏移量向后偏移
            memcpy(name,data.c_str()+offset,sizeof(name));  //将字符串中的客户端名字解析出来
            offset+=sizeof(name);
            memcpy(text,data.c_str()+offset,sizeof(text));  //将字符串中的消息正文解析出来
        }
    };

    //客户端结构体类型
    struct Client
    {
        int fd;     //客户端套接字
        struct sockaddr_in cin;     //客户端地址信息结构体
    };

private:
    int sfd;        //服务器套接字
    vector<Client>clients;  //在线的客户端列表
    mutex client_mutex;     //保护clients 客户端列表的互斥锁

    //线程池相关变量
    //线程池相关的私有成员
    vector<thread> workers;         //存储工作的线程
    queue<function<void()>> tasks;  //存储任务的队列
    mutex task_mutex;               //互斥锁
    condition_variable task_cv;     //用于通知线程有新任务的条件变量
    bool stop;                      //线程池的标志，判断线程池是否停止

    void errLog(const char*msg);        //错误信息日志函数
    void startThreadPool(size_t numThreads);    //启动线程池函数
    void addTask(function<void ()>task);     //将任务添加到线程池中

public:
    chatServer(const char*ip,int port,size_t threadPoolSize=4); //构造函数声明
    ~chatServer();      //析构函数声明
    void run();         //运行服务器函数
    void handleClient(int client_fd,struct sockaddr_in cin);    //处理客户端连接的函数
    void broadcast(const MSG &msg,int exclude_fd=-1);       //广播消息函数
};


#endif