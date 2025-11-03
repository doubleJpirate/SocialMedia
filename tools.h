#pragma once

#include<sys/epoll.h>

void setNonBlock(int fd);

void addWaitFd(int epoll,int fd);

void deletWaitFd(int epoll,int fd);