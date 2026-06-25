#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <myhead.h>
#include <thread>
#define N 128       //消息缓冲区的大小
#define LOGIN 1     //表示登录消息类型
#define CHAT 2      //表示聊天消息类型
#define QUIT 3      //表示退出消息类型

//定义客户端
class ChatClient
{
    //定义一些公共类型
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

    //类中私有成员
private:
    int cfd;    //客户端套接字
    string name;    //客户端名称
    bool running;   //判断客户端是否允许的标志

    void errLog(const char*msg);    //错误日志函数
    void sendMsg(int type,const string &text="");   //发送消息函数
    void recvMsg();           //接收消息函数

public:
    ChatClient(const char*ip,int port,const string &name); //有参构造
    ~ChatClient();

    void run();     //客户端运行函数
};

#endif