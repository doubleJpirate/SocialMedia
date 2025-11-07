#pragma once

#include<sys/epoll.h>



// 网络对象，用于处理所有网络事件
class WebServer
{
public:
    WebServer();
    void init(int port);
    void start();
    ~WebServer();
private:
    void initSocket(int port);
    void initEpoll();
private:
    int m_listenSocket;
    int m_epoll;
    epoll_event m_epollEvent[1024]{};
};