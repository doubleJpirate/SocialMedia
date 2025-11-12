#pragma once

#include<sys/epoll.h>
#include<string>

//多个类需要使用的函数
//设置为全局函数

void setNonBlock(int fd);

void addWaitFd(int epoll,int fd);

void deletWaitFd(int epoll,int fd);

int hexCharToInt(char c);

std::string urlDecode(std::string& encode);