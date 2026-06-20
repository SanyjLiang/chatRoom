#include "chatServer.h"
#include <myhead.h>

int main(int argc,const char*argv[])
{
    //判断传参是否正确
    if(argc<3)
    {
        cout<<"请输入 ip 地址和 端口号"<<endl;
        return -1;
    }

    //传参正确，构造一个服务器
    chatServer server(argv[1],atoi(argv[2]));

    //启动服务器
    server.run();

    return 0;
}