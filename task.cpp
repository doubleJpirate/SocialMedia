#include "task.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sstream>
#include <fstream>

#include "threadpool.h"
#include "tools.h"

void ListenTask::process()
{
    int clientfd = accept(m_fd, nullptr, nullptr);
    setNonBlock(clientfd);
    addWaitFd(m_epoll, clientfd);
    delete this;
}

void readTask::process()
{
    char buf[1024]{};
    int ret = recv(m_fd, buf, sizeof(buf), 0);
    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 非阻塞状态，暂时无数据到达
            delete this;
            return;
        }
        deletWaitFd(m_epoll, m_fd);
        close(m_fd);
        delete this;
        return;
    }
    else if (ret == 0)
    {
        // 客户端主动断开连接
        deletWaitFd(m_epoll, m_fd);
        close(m_fd);
        delete this;
        return;
    }
    // std::cout<<"buf:"<<buf<<std::endl;
    std::string recvMsg(buf);
    handle(recvMsg);
    delete this;
}

void readTask::handle(std::string recvMsg)
{
    /*示例http请求报文
    POST /api/login HTTP/1.1\r\n
    Host: example.com\r\n
    User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.1 Safari/605.1.15\r\n
    Content-Type: application/json\r\n
    Content-Length: 45\r\n
    Accept: application/json\r\n
    Connection: close\r\n
    \r\n
    {"username":"admin","password":"123456","remember":true}\r\n*/

    // 当前位置
    size_t pos = 0;
    // 拆分请求行
    size_t index = recvMsg.find("\r\n", 0);
    std::string request_line = recvMsg.substr(0, index);
    std::istringstream iss(request_line);
    iss >> m_method;
    iss >> m_path;
    iss >> m_version; // 该变量在该项目中没有作用
    while (1)
    {
        pos = index + 2;
        index = recvMsg.find("\r\n", pos);
        if (index == pos)
            break; // 检测到空行退出
        std::string tmp = recvMsg.substr(pos, index - pos);
        size_t colon = tmp.find(": ", 0); // 查找冒号
        std::string key = tmp.substr(0, colon);
        std::string value = tmp.substr(colon + 2);
        m_headers[key] = value;
    }
    pos += 2;
    if (pos < recvMsg.size())
    {
        m_body = recvMsg.substr(pos);
    }
    // TODO:后续任务分发
    if(m_method=="GET")
    {
        Task* task = new writeTask(m_epoll,m_fd);
        ThreadPool::addTask(task);
    }
}

void writeTask::process()
{
    std::ifstream ifs("entry.html", std::ios::binary);
    //将整个文件内容读入到string中
    std::string htmlcontent(std::istreambuf_iterator<char>(ifs),{});//两个参数分别为创建迭代器，默认迭代器
    std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "+
        std::to_string(htmlcontent.size())+"\r\n\r\n"+htmlcontent;
    send(m_fd,msg.c_str(), msg.size(), 0);
    delete this;
}
