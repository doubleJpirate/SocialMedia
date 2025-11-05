#pragma once

//任务基类
class Task{
public:
    Task(int ep,int fd):m_epoll(ep),m_fd(fd){

    }
    virtual void process() = 0;
protected:
    int m_epoll;
    int m_fd;
};



class ListenTask:public Task{
public:
    ListenTask(int ep,int fd):Task(ep,fd){

    }
    virtual void process()override;

};

class readTask:public Task{
public:
    readTask(int ep,int fd):Task(ep,fd){

    }
    virtual void process()override;
};

class writeTask:public Task{
public:
    writeTask(int ep,int fd):Task(ep,fd){

    }
    virtual void process()override;
};