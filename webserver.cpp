#include "webserver.h"

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"tools.h"
#include"threadpool.h"

WebServer::WebServer()
{
}

//初始化listen套接字和epoll
void WebServer::init(int port)
{
    //暂时不添加错误检验
    initSocket(port);
    initEpoll();
    addWaitFd(m_epoll,m_listenSocket);
}

//通信逻辑
void WebServer::start()
{
    Task* task;
    while(1)
    {
        int resNum = epoll_wait(m_epoll,m_epollEvent,1024,-1);
        for(int i = 0;i<resNum;i++)
        {
            int resFd = m_epollEvent[i].data.fd;
            if(resFd==m_listenSocket)
            {
                task = new ListenTask(m_epoll,resFd);
            }
            else if(m_epollEvent[i].events&EPOLLIN)
            {
                task = new readTask(m_epoll,resFd);
            }
            if(task)
            {
                ThreadPool::addTask(task);
                task = nullptr;
            }
        }
    }
}

WebServer::~WebServer()
{
}

void WebServer::initSocket(int port)
{
    m_listenSocket = socket(AF_INET,SOCK_STREAM,0);

    int reuse = 1;//设置地址重用
    setsockopt(m_listenSocket,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(m_listenSocket,(sockaddr*)&addr,sizeof(addr));

    listen(m_listenSocket,1024);

    setNonBlock(m_listenSocket);
}

void WebServer::initEpoll()
{
    m_epoll = epoll_create(1);
}



