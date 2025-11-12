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
private:
    void handle(std::string recvMsg);
    void userLogin();
    void userRegister();

private:
    std::string m_method;//以下三个为http报文请求头
    std::string m_path;
    std::string m_version;
    std::unordered_map<std::string, std::string> m_headers;//请求体哈希表
    std::string m_body;//主体体
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
private:
    void sendLoginHtml();
    void sendRegisRes();
    void sendLogRes();
    void sendMainHtml();
    
private:
    int m_type;//表示回复消息类型
    int m_status;//表示状态，0为正常
    std::string m_msg;//存储相关信息
};