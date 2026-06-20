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
    //定义一个消息类型的变量，用于接收客户端消息
    MSG msg;        //处理后的消息形态
    char buffer[sizeof(MSG)];       //杠接收到的消息形态

    //循环接收客户端消息
    while(true)
    {
        //读取客户端消息
        int recv_len=recv(client_fd,buffer,sizeof(buffer),0);

        //当客户端下线或者recv函数错误
        if(recv_len<=0)
        {
            //表示客户端下线
            //遍历客户端容器
            auto it=clients.begin();    //定义遍历客户端容器的迭代器
            while(it!=clients.end())
            {
                if(it->fd==client_fd)   //在容器中找到当前套接字
                {
                    clients.erase(it);  //将当前套接字从列表中移除
                    break;
                }
                ++it;   //指针向后偏移
            }
            close(client_fd);   //关闭套接字
            break;
        }

        //程序执行至此，表示收到某个客户端消息，需要进行反序列化
        msg.deseralize(string(buffer,recv_len));

        //对消息类型进行判断
        switch(ntohl(msg.type))
        {
            case LOGIN:
            {
                //表示客户端发的是登录消息
                //定义一个互斥锁，保护客户端容器
                unique_lock<mutex> lock(client_mutex);
                Client new_client;
                //将新客户端的相关信息获取下来
                new_client.fd=client_fd;
                new_client.cin=cin;

                //将新客户端放入客户端容器中
                clients.push_back(new_client);

                //组装一条消息，广播给所有客户端
                sprintf(msg.text,"-----%s 登录成功 -----",msg.name);
                broadcast(msg);     //告诉所有用户，包括自己，该用户登录成功
                break;
            }

            case CHAT:
            {
                //表示客户端发的是聊天消息
                //定义一个互斥锁，保护客户端容器
                unique_lock<mutex> lock(client_mutex);
                broadcast(msg,client_fd);       //除了自己，将消息转发给其他所有人

                break;
            }

            case QUIT:
            {
                //表示客户端发的是退出消息
                //定义一个互斥锁，保护客户端容器
                unique_lock<mutex>lock(client_mutex);
                auto it=clients.begin();    //定义迭代器，遍历客户端容器
                while(it!=clients.end())
                {
                    if(it->fd==client_fd)
                    {
                        clients.erase(it);      //将当前用户清除容器
                        break;
                    }

                    ++it;       //继续遍历下一个
                }

                //将消息广播给所有人
                sprintf(msg.text,"--------%s 退出聊天室 -------",msg.name);
                broadcast(msg);
                close(client_fd);   //关闭当前客户端套接字
                break;
            }

            default:
                cout<<"消息类型有误"<<endl;
                break;
        }
    }
}

//定义广播函数
void chatServer::broadcast(const MSG&msg,int exclude_fd)
{
    //将要广播的消息进行序列化操作，转化成字符串
    string data = msg.serialize();

    //将字符串转发给所有客户端
    for(const auto&client:clients)
    {
        if(client.fd!=exclude_fd)
        {
            if(send(client.fd,data.c_str(),data.size(),0)<0)
            {
                perror("send error");
                return ;
            }
        }
    }
}