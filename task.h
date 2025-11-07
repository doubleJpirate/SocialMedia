#pragma once

#include <string>
#include <unordered_map>

// class Message{
// public:
// };

// 任务基类
class Task
{
public:
    Task(int ep, int fd) : m_epoll(ep), m_fd(fd)
    {
    }
    virtual void process() = 0;

protected:
    int m_epoll;
    int m_fd;
};

class ListenTask : public Task
{
public:
    ListenTask(int ep, int fd) : Task(ep, fd)
    {
    }
    virtual void process() override;
};

class readTask : public Task
{
public:
    readTask(int ep, int fd) : Task(ep, fd)
    {
    }
    virtual void process() override;
    void handle(std::string recvMsg);
    void userLogin();
    void userRegister();

private:
    std::string m_method;
    std::string m_path;
    std::string m_version;
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_body;
};

class writeTask : public Task
{
public:
    writeTask(int ep, int fd,int type,int status,std::string msg) : Task(ep, fd)
    {
        m_type = type;
        m_msg = msg;
        m_status = status;
    }
    virtual void process() override;
    void sendLoginHtml();
    void sendRegisRes();
    void sendLogRes();
private:
    int m_type;
    int m_status;
    std::string m_msg;
};