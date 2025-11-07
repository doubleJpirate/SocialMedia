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
#include "database.h"

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
    std::cout<<"buf:"<<buf<<std::endl;
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
        if(m_path=="/")
        {
            Task* task = new writeTask(m_epoll,m_fd,0,0,"");
            ThreadPool::addTask(task);
        }
    }
    else if(m_method=="POST")
    {
        if(m_path=="/api/login")
        {
            userLogin();
        }
        else if(m_path=="/api/register")
        {
            userRegister();
        }
    }
}

void readTask::userLogin()
{
    //user=111&password=aaa
    //这里的user实际上还可能是email
    std::string user,pwd;
    int pindex = m_body.find("&password=");
    user = m_body.substr(5,pindex-5);
    pwd = m_body.substr(pindex+10);
    std::string selsql = "SELECT id FROM `User` WHERE username = '"+user+"' AND password = '"+pwd+"';";
    auto result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if(!result.empty()&&!result["id"].empty())
    {
        Task* task = new writeTask(m_epoll,m_fd,2,0,"");
        ThreadPool::addTask(task);
        return;
    }
    selsql = "SELECT id FROM `User` WHERE email = '"+user+"' AND password = '"+pwd+"';";
    result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if(!result.empty()&&!result["id"].empty())
    {
        Task* task = new writeTask(m_epoll,m_fd,2,0,"");
        ThreadPool::addTask(task);
        return;
    }
    Task* task = new writeTask(m_epoll,m_fd,2,1,"");
    ThreadPool::addTask(task);
}

void readTask::userRegister()
{
    //user=111&email=ttt.com&password=aaa
    std::string user,email,pwd;
    int eindex = m_body.find("&email=");
    user = m_body.substr(5,eindex-5);
    int pindex = m_body.find("&password=");
    email = m_body.substr(eindex+7,pindex-eindex-7);
    pwd = m_body.substr(pindex+10);
    std::string selsql = "SELECT id FROM `User` WHERE username = '"+user+"' OR email = '"+email+"';";
    auto result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if(!result.empty()&&!result["id"].empty())
    {
        Task* task = new writeTask(m_epoll,m_fd,1,1,"");
        ThreadPool::addTask(task);
        return;
    }
    // 下面的语句存在sql注入风险，但是只是作为练手项目，暂不考虑这些问题
    std::string inssql = "INSERT INTO `User` (username, password, email) VALUES ('"+user+"', '"+pwd+"', '"+email+"');";
    DataBase::getInstance()->executeSQL(inssql.c_str());
    Task* task = new writeTask(m_epoll,m_fd,1,0,"");
    ThreadPool::addTask(task);
}

void writeTask::process()
{
    if(m_type==0)sendLoginHtml();
    else if(m_type==1)sendRegisRes();
    else if(m_type==2)sendLogRes();
    delete this;
}

void writeTask::sendLoginHtml()
{
    std::ifstream ifs("entry.html", std::ios::binary);
    //将整个文件内容读入到string中
    std::string htmlcontent(std::istreambuf_iterator<char>(ifs),{});//两个参数分别为创建迭代器，默认迭代器
    ifs.close(); 
    std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "+
        std::to_string(htmlcontent.size())+"\r\n\r\n"+htmlcontent;
    send(m_fd,msg.c_str(), msg.size(), 0);
}

void writeTask::sendRegisRes()
{
    std::string regisRes,content;
    if(m_status==0){
        regisRes+="HTTP/1.1 200 OK\r\n";
        content = R"({
    "success": true,
    "message": "注册成功"
    })";
    }
    else{
        regisRes+="HTTP/1.1 400 Bad Request\r\n";
        content = R"({
  "success": false,
  "message": "用户名已存在"
    })";
    }
    regisRes+="Content-Type: application/json; charset=utf-8\r\n";
    regisRes+="Content-Length: "+std::to_string(content.size())+"\r\n\r\n";
    regisRes+=content;
    send(m_fd,regisRes.c_str(),regisRes.size(),0);
}

void writeTask::sendLogRes()
{
    if(m_status){
        std::string logRes,content;
        logRes = "HTTP/1.1 401 Unauthorized\r\n";
        content = "{\"code\":401,\"msg\":\"failed\"}";
        logRes+="Content-Type: application/json; charset=utf-8\r\n";
        logRes+="Content-Length: "+std::to_string(content.size())+"\r\n\r\n";
        logRes+=content;
        send(m_fd,logRes.c_str(),logRes.size(),0);
    }
    else{
        std::string mainEdge;
    std::string html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>登录成功</title>
    <style>
        body { text-align: center; margin-top: 100px; font-family: sans-serif; }
        h1 { color: green; }
    </style>
</head>
<body>
    <h1>登录成功</h1>
    <p>您已成功登录系统</p>
    <a href="/">回到首页</a>
</body>
</html>)";
    mainEdge += "HTTP/1.1 200 OK\r\n";
    mainEdge += "Content-Type: text/html; charset=utf-8\r\n";
    mainEdge += "Content-Length: " + std::to_string(html.size()) + "\r\n\r\n";
    mainEdge += html;

    send(m_fd, mainEdge.c_str(), mainEdge.size(), 0);
    }
}
