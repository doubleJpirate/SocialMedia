#include "tools.h"

#include<fcntl.h>

void setNonBlock(int fd)
{
    int flag = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,flag|FNONBLOCK);
}

void addWaitFd(int epoll, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    epoll_ctl(epoll,EPOLL_CTL_ADD,fd,&event);
}

void deletWaitFd(int epoll, int fd)
{
    epoll_ctl(epoll,EPOLL_CTL_DEL,fd,nullptr);
}
