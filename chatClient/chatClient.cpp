#include "chatClient.h"

//构造函数的定义
 ChatClient::ChatClient(const char*ip,int port,const string &name)
 {
    //创建客户端套接字
    cfd=socket(AF_INET,SOCK_STREAM,0);
    if(cfd<0)
    {
        perror("socket error");
        return;
    }

    //连接服务器
    //1.初始化地址信息结构体
    struct sockaddr_in sin;
    sin.sin_family=AF_INET;        //通信域
    sin.sin_port = htons(port);     //端口号
    sin.sin_addr.s_addr=inet_addr(ip);   //服务器ip

    //2.连接服务器
    if(connect(cfd,(struct sockaddr*)&sin,sizeof(sin))<0)
    {
        perror("connect error");
        return ;
    }

    //程序运行至此，表示连接服务器成功
    //顺便向服务器发送登录消息
    sendMsg(LOGIN);
    
 }

//析构函数的定义
 ChatClient::~ChatClient()
 {
    sendMsg(QUIT);  //向服务器发送退出消息
    close(cfd);     //关闭套接字
 }