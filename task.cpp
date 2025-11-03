#include"task.h"

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<iostream>
#include<cstring>
#include<unistd.h>
#include<errno.h>

#include"threadpool.h"
#include"tools.h"


void ListenTask::process()
{
    int clientfd = accept(m_fd,nullptr,nullptr);
    setNonBlock(clientfd);
    addWaitFd(m_epoll,clientfd);
    delete this;
}

void readTask::process()
{
    char buf[1024]{};
    int ret = recv(m_fd,buf,sizeof(buf),0);
    if(ret<0){
        if(errno==EAGAIN||errno==EWOULDBLOCK){
            delete this;
            return;
        }
        deletWaitFd(m_epoll,m_fd);
        close(m_fd);
        delete this;
        return;
    }
    else if(ret==0){
        deletWaitFd(m_epoll,m_fd);
        close(m_fd);
        delete this;
        return;
    }
    std::cout<<"buf:"<<buf<<std::endl;
    Task* task = new writeTask(m_epoll,m_fd);
    ThreadPool::addTask(task);
    delete this;
}

void writeTask::process()
{
    const char* msg = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\nConnection: close\r\n\r\nHello";
    send(m_fd,msg,strlen(msg),0);
    delete this;
}
