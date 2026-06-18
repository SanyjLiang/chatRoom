#include "chatServer.h"

//构造函数的定义：初始化服务器，创建套接字，绑定地址信息结构体，启动监听
chatServer::chatServer(const char*ip,int port,size_t threadPoolSize):stop(false)
{
    //创建服务器套接字
    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1)
    {
        perror("socket error");
        return;
    }

    //端口号快速重用
    int reuse=1;
    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0)
    {
        perror("setsockopt error");
        return;
    }

    //填充地址信息结构体
    struct sockaddr_in sin;
    sin.sin_family=AF_INET;     //通信域  ipv4
    sin.sin_port=htons(port);   //端口号
    sin.sin_addr.s_addr=inet_addr(ip);  //ip地址

    //绑定地址信息结构体
    if(bind(sfd,( struct sockaddr*)&sin,sizeof(sin))==-1)
    {
        perror("bind error");
        return ;
    }

    //启动监听
    if(listen(sfd,10)<0)
    {
        perror("listen error");
        return;
    }

    //启动线程池
    startThreadPool(threadPoolSize);
}

//析构函数的定义
chatServer::~chatServer()
{
    stop=true;      //设置停止标志，表示停止线程池
    task_cv.notify_all();       //唤醒所有任务
    for(auto&worker:workers)
    {
        worker.join();      //等待回收所有线程
    }

    close(sfd);     //关闭服务器套接字
}

//启动线程池函数的定义
void chatServer::startThreadPool(size_t numThreads)
{
    //循环创建numThreads个线程
    for(int i=0;i<numThreads;i++)
    {
        //创建一个线程，并且将线程放入到线程容器中
        workers.emplace_back([this]{        //使用lambda表达式定义线程体
            while(true)
            {
                function<void()> task;      //创建一个任务
                {
                    unique_lock<mutex> lock(task_mutex);    //加锁，保护任务队列
                    task_cv.wait(lock,[this]{               //等待条件变量通知
                        return stop || !tasks.empty();      //当线程池停止工作或者任务队列
                    });

                    if(stop&&tasks.empty())
                    {
                        return;     //当线程池停止并且任务队列为空，退出线程
                    }

                    task=move(tasks.front()); //从任务队列中取出一个任务
                    tasks.pop();            //将任务从任务队列中移除
                }
                task();     //执行任务
            }
        });
    }
}

//将任务添加到线程池中
void chatServer::addTask(function<void()> task)
{
    {
        unique_lock<mutex>lock(task_mutex); //获取锁资源，保护条件变量
        tasks.push(task);       //将任务添加到任务队列
    }

    //唤醒一个等待的线程去开始工作
    task_cv.notify_one();
}

//打印错误日志函数的定义
void chatServer::errLog(const char *msg)
{
    cerr<<__FILE__<<"  "<<__func__<<"  "<<__FILE__<<endl;
    perror(msg);
}

//启动服务器函数的定义
void chatServer::run()
{
    //循环等待客户端连接请求
    while(true)
    {
        struct sockaddr_in cin;         //用于接收客户端的地址信息结构体
        socklen_t clen=sizeof(cin);     //客户端地址信息的长度

        //阻塞接收客户端连接请求
        int client_fd=accept(sfd,(struct sockaddr*)&cin,&clen);
        if(client_fd<0)
        {
            errLog("accept errror");
            continue;   //继续接收下一个
        }

        //将处理客户端信息的任务，添加到线程池
        addTask([this,client_fd,cin]{
            //函数体就是处理客户端的功能函数
            this->handleClient(client_fd,cin);
        });
    }
}

//处理客户端消息的函数定义
void chatServer::handleClient(int client_fd,struct sockaddr_in cin)
{

}