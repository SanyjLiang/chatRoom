#include "chatClient.h"

int main(int argc,const char*argv[])
{
    //确保执行时，外部传入了ip地址，端口号和用户名
    if(argc<4)
    {
        cout<<"请输入  ip   端口号  用户名"<<endl;
        return -1;
    }

    //实例化一个客户端
    ChatClient client(argv[1],atoi(argv[2]),argv[4]);

    //运行客户端
    client.run();
    return 0;
}