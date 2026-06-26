#include "chatClient.h"

//构造函数的定义
 ChatClient::ChatClient(const char*ip,int port,const string &name):name(name),running(true)
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


void ChatClient::run()
{
    //创建一个分支线程，用于接收服务端发来的消息
    thread recvThread(&ChatClient::recvMsg,this);

    //主线程，用于向服务端发送消息
    string text;        //存储要发送的消息
    while(running)
    {
        //从终端读取用户要发送的消息
        getline(cin,text);

        //判断是否位退出消息
        if(text=="quit")
        {
            //客户端退出状态
            running = false;
            //退出消息在析构中发送
            break;
        }

        //调用发送消息函数
        sendMsg(CHAT,text);
    }

    recvThread.join();      //回收线程资源
}

//定义输出错误日志函数
void ChatClient:: errLog(const char*msg)
{
    //输出错误的文件，功能，对应的错误行
    cerr<<__FILE__<<" "<<__func__<<" "<<__LINE__<<endl;
    perror(msg);
}

//定义向服务器发送消息的函数
void ChatClient::sendMsg(int type,const string &text)
{
    MSG msg;                                //要发送的数据结构体
    msg.type=htonl(type);                    //消息类型
    strncpy(msg.name,name.c_str(),sizeof(msg.name));        //设置客户端用户名
    strncpy(msg.text,text.c_str(),sizeof(msg.text));        //消息正文

    //将要发送的消息进行序列化处理
    string data=msg.serialize();

    //将转换的数据发送给服务器
    if(send(cfd,data.c_str(),data.size(),0)<0)
    {
        errLog("send error");
        return;
    }

    cout<<"消息发送成功"<<endl;
}

//定义接收服务器发来消息的函数
void ChatClient::recvMsg()
{
    //用于接收消息
    char buffer[sizeof(MSG)];
    while(running)
    {
        //非阻塞接收客户端消息
        int recv_len=recv(cfd,buffer,sizeof(buffer),MSG_DONTWAIT);
        if(recv_len<0)
        {
            if(errno==EAGAIN)
            {
                //休眠100s
                usleep(100000);
            }
            else
            {
                //错误
                errLog("recv error");
                running = false;
                break;
            }
            
        }
        else if(recv_len==0)
        {
            //错误
            errLog("recv error");
            running = false;
            break;      //连接关闭
        }
        else
        {
             //将消息进行解析
        MSG msg;
        msg.deseralize(string(buffer,recv_len));       //反序列化
        cout<<msg.name<<": "<<msg.text<<endl;
        }

       
    }
}    