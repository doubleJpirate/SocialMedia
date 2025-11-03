#pragma once

#include<sys/epoll.h>



// 网络对象，用于处理所有网络事件
class WebServer{
public:
    WebServer();
    void init();
    void start();
    ~WebServer();
private:
    void initSocket();
    void initEpoll();
private:
    int m_listenSocket;
    int m_epoll;
    epoll_event m_epollEvent[1024]{};
};