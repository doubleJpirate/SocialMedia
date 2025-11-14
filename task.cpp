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
#include <string>

#include "threadpool.h"
#include "tools.h"
#include "database.h"

//临时编译指令g++ -std=c++11 *.o task.cpp -o text -pthread -lmysqlclient

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
        if(m_path=="/")//检测到首页面的get请求
        {
            Task* task = new writeTask(m_epoll,m_fd,0,0,"");
            ThreadPool::addTask(task);
        }
        else if(m_path.substr(0,5)=="/home")
        {
            Task* task = new writeTask(m_epoll,m_fd,3,0,"");
            ThreadPool::addTask(task);
        }
        else if(m_path.substr(0,4)=="/img")
        {
            Task* task = new writeTask(m_epoll,m_fd,5,0,m_path.substr(4));
            ThreadPool::addTask(task);
        }
    }
    else if(m_method=="POST")
    {
        if(m_path=="/api/login")//登录请求
        {
            userLogin();
        }
        else if(m_path=="/api/register")//注册请求
        {
            userRegister();
        }
        else if(m_path=="/api/find")
        {
            findMsg();
        }
        else if(m_path=="/api/attention")
        {
            attentionMsg();
        }
        else if(m_path=="/api/mypost")
        {
            myMsg();
        }
        else if(m_path=="/api/publish")
        {
            publish();
        }
        else if(m_path=="/api/like")
        {
            addLike();
        }
        else if(m_path=="/api/unlike")
        {
            delLike();
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
    user = urlDecode(user);
    pwd = m_body.substr(pindex+10);
    std::string selsql = "SELECT id FROM `User` WHERE username = '"+user+"' AND password = '"+pwd+"';";
    auto result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if(!result.empty()&&!result["id"].empty())//查询到用户名密码相符
    {
        Task* task = new writeTask(m_epoll,m_fd,2,0,result["id"][0]);
        ThreadPool::addTask(task);
        return;
    }
    selsql = "SELECT id FROM `User` WHERE email = '"+user+"' AND password = '"+pwd+"';";
    result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if(!result.empty()&&!result["id"].empty())//查询到邮箱密码相符
    {
        Task* task = new writeTask(m_epoll,m_fd,2,0,result["id"][0]);
        ThreadPool::addTask(task);
        return;
    }
    //无相符内容
    Task* task = new writeTask(m_epoll,m_fd,2,1,"");
    ThreadPool::addTask(task);
}

void readTask::userRegister()
{
    //user=111&email=ttt.com&password=aaa
    std::string user,email,pwd;
    int eindex = m_body.find("&email=");
    user = m_body.substr(5,eindex-5);
    user = urlDecode(user);
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

void readTask::findMsg()
{
    //page=...&id=...
    int index = m_body.find("&id=");
    std::string page = m_body.substr(5,index-5);
    std::string id = m_body.substr(index+4);

    int npage = std::stoi(page);
    //查询消息总数量
    std::string sql = "select count(*) as cnt from Message;";
    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string s(res["cnt"][0]);
    int cnt = std::stoi(s);
    //向上取整分页
    int allpage = cnt/3+(cnt%3!=0);
    //每三组数据分为一组，查询第page组数据
    sql = "SELECT username,headimg,txt,likes,comments,m.id,CASE WHEN l.msgid IS NOT NULL THEN 1 ELSE 0 END AS isliked FROM `Message` m LEFT JOIN `User` u ON m.authorid = u.id LEFT JOIN `Likes` l ON m.id = l.msgid AND l.userid = "+id+" ORDER BY m.id DESC LIMIT 3 OFFSET "+std::to_string((npage-1)*3)+";";
    res = DataBase::getInstance()->executeSQL(sql.c_str());

    std::string backMsg = "{\n\"data\":[\n";
    // 返回数据示例
//     {
//      "data": [
//      {
//           "author": "测试",
//          "avatar": "/img/test.png",
//          "content": "周末去了这家餐厅，味道超赞",
//          "likes": 76,
//          "comments": 12,
//          "msgid": 1001,
//          "isLiked": false
//      },
//      {
//          "author": "是一对J",
//          "avatar": "/img/user1.png",
//          "content": "大家有什么好的健身方法推荐吗？",
//          "likes": 33,
//          "comments": 9,
//          "msgid": 1002,
//          "isLiked": true
//      }
//      ],
//      "totalPages": 5  // 总页数不变，前端分页逻辑保持原样
//      }
    int n = res["username"].size();
    for(int i = 0;i<n;i++)
    {
        backMsg+="{\n\"author\": \"";
        backMsg+=res["username"][i];
        backMsg+="\",\n";
        backMsg+="\"avatar\": \"";
        backMsg+="http://192.168.88.101:19200";//需更换为实际服务器ip
        backMsg+=res["headimg"][i];
        backMsg+="\",\n";
        backMsg+="\"content\": \"";
        backMsg+=res["txt"][i];
        backMsg+="\",\n";
        backMsg+="\"likes\": ";
        backMsg+=res["likes"][i];
        backMsg+=",\n";
        backMsg+="\"comments\": ";
        backMsg+=res["comments"][i];
        backMsg+=",\n";
        backMsg+="\"msgid\": ";
        backMsg+=res["id"][i];
        backMsg+=",\n";
        backMsg+="\"isLiked\": ";
        backMsg+=res["isliked"][i];
        backMsg+="\n";
        backMsg+="}";
        if(i!=n-1)backMsg+=",";
        backMsg+="\n";
    }
    backMsg+="],\n\"totalPages\": "+std::to_string(allpage)+"\n}";

    Task* task = new writeTask(m_epoll,m_fd,4,0,backMsg);
    ThreadPool::addTask(task);
}

void readTask::attentionMsg()
{
    int index = m_body.find("&id=");
    std::string page = m_body.substr(5,index-5);
    std::string id = m_body.substr(index+4);
    std::string sql = "select count(*) as cnt from Message m inner join Follows f on m.authorid = f.followed_id where f.follower_id = "+id+";";
    int npage = std::stoi(page);
    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string s(res["cnt"][0]);
    int cnt = std::stoi(s);
    int allpage = cnt/3+(cnt%3!=0);
    sql = "SELECT u.username,u.headimg,m.txt,m.likes,m.comments,m.id,CASE WHEN l.msgid IS NOT NULL THEN 1 ELSE 0 END AS isliked FROM `Message` m INNER JOIN `User` u ON m.authorid = u.id INNER JOIN `Follows` f ON f.follower_id = "+id+" AND m.authorid = f.followed_id LEFT JOIN `Likes` l ON m.id = l.msgid AND l.userid = "+id+" ORDER BY m.id DESC LIMIT 3 OFFSET "+std::to_string((npage-1)*3)+";";
    res = DataBase::getInstance()->executeSQL(sql.c_str());

    std::string backMsg = "{\n\"data\":[\n";
    
    int n = res["username"].size();
    for(int i = 0;i<n;i++)
    {
        backMsg+="{\n\"author\": \"";
        backMsg+=res["username"][i];
        backMsg+="\",\n";
        backMsg+="\"avatar\": \"";
        backMsg+="http://192.168.88.101:19200";//需更换为实际服务器ip
        backMsg+=res["headimg"][i];
        backMsg+="\",\n";
        backMsg+="\"content\": \"";
        backMsg+=res["txt"][i];
        backMsg+="\",\n";
        backMsg+="\"likes\": ";
        backMsg+=res["likes"][i];
        backMsg+=",\n";
        backMsg+="\"comments\": ";
        backMsg+=res["comments"][i];
        backMsg+=",\n";
        backMsg+="\"msgid\": ";
        backMsg+=res["id"][i];
        backMsg+=",\n";
        backMsg+="\"isLiked\": ";
        backMsg+=res["isliked"][i];
        backMsg+="\n";
        backMsg+="}";
        if(i!=n-1)backMsg+=",";
        backMsg+="\n";
    }
    backMsg+="],\n\"totalPages\": "+std::to_string(allpage)+"\n}";

    Task* task = new writeTask(m_epoll,m_fd,4,0,backMsg);
    ThreadPool::addTask(task);
}

void readTask::myMsg()
{
   int index = m_body.find("&id=");
    std::string page = m_body.substr(5,index-5);
    std::string id = m_body.substr(index+4);
    std::string sql = "select count(*) as cnt from Message where authorid = "+id+";";
    int npage = std::stoi(page);
    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string s(res["cnt"][0]);
    int cnt = std::stoi(s);
    int allpage = cnt/3+(cnt%3!=0);
    sql = "SELECT username,headimg,txt,likes,comments,m.id,CASE WHEN l.msgid IS NOT NULL THEN 1 ELSE 0 END AS isliked FROM `Message` m LEFT JOIN `User` u ON m.authorid = u.id LEFT JOIN `Likes` l ON m.id = l.msgid AND l.userid = "+id+" WHERE m.authorid = "+id+" ORDER BY m.id DESC LIMIT 3 OFFSET "+std::to_string((npage-1)*3)+";";
    res = DataBase::getInstance()->executeSQL(sql.c_str());

    std::string backMsg = "{\n\"data\":[\n";
    
    int n = res["username"].size();
    for(int i = 0;i<n;i++)
    {
        backMsg+="{\n\"author\": \"";
        backMsg+=res["username"][i];
        backMsg+="\",\n";
        backMsg+="\"avatar\": \"";
        backMsg+="http://192.168.88.101:19200";//需更换为实际服务器ip
        backMsg+=res["headimg"][i];
        backMsg+="\",\n";
        backMsg+="\"content\": \"";
        backMsg+=res["txt"][i];
        backMsg+="\",\n";
        backMsg+="\"likes\": ";
        backMsg+=res["likes"][i];
        backMsg+=",\n";
        backMsg+="\"comments\": ";
        backMsg+=res["comments"][i];
        backMsg+=",\n";
        backMsg+="\"msgid\": ";
        backMsg+=res["id"][i];
        backMsg+=",\n";
        backMsg+="\"isLiked\": ";
        backMsg+=res["isliked"][i];
        backMsg+="\n";
        backMsg+="}";
        if(i!=n-1)backMsg+=",";
        backMsg+="\n";
    }
    backMsg+="],\n\"totalPages\": "+std::to_string(allpage)+"\n}";

    Task* task = new writeTask(m_epoll,m_fd,4,0,backMsg);
    ThreadPool::addTask(task);
}

void readTask::publish()
{
    //id=...&text=...
    int index = m_body.find("&text=");
    std::string id = m_body.substr(3,index-3);
    std::string text = m_body.substr(index+6);
    text = urlDecode(text);
    if(text.empty())
    {
        Task* task = new writeTask(m_epoll,m_fd,6,1,"");
        ThreadPool::addTask(task);
        return;
    }
    std::string sql = "INSERT INTO `Message` (`txt`, `authorid`) VALUES ('"+text+"', "+id+");";

    DataBase::getInstance()->executeSQL(sql.c_str());
    Task* task = new writeTask(m_epoll,m_fd,6,0,"");
    ThreadPool::addTask(task);
}

void readTask::addLike()
{
    //userid=...&msgid=...
    int index = m_body.find("&msgid=");
    std::string userid = m_body.substr(7,index-7);
    std::string msgid = m_body.substr(index+7);
    std::string sql = "INSERT INTO `Likes` (`msgid`, `userid`) VALUES ("+msgid+", "+userid+");";

    DataBase::getInstance()->executeSQL(sql.c_str());

    sql = "UPDATE `Message` SET `likes` = `likes` + 1 WHERE `id` = "+msgid+";";

    DataBase::getInstance()->executeSQL(sql.c_str());
}

void readTask::delLike()
{
    //userid=...&msgid=...
    int index = m_body.find("&msgid=");
    std::string userid = m_body.substr(7,index-7);
    std::string msgid = m_body.substr(index+7);

    std::string sql = "DELETE FROM `Likes` WHERE `msgid` = "+msgid+" AND `userid` = "+userid+";";

    DataBase::getInstance()->executeSQL(sql.c_str());

    sql = "UPDATE `Message` SET `likes` = `likes` - 1 WHERE `id` = "+msgid+";";

    DataBase::getInstance()->executeSQL(sql.c_str());
}

void writeTask::process()
{
    if(m_type==0)
        sendLoginHtml();
    else if(m_type==1)
        sendRegisRes();
    else if(m_type==2)
        sendLogRes();
    else if(m_type==3)
        sendMainHtml();
    else if(m_type==4)
        sendFindMsg();
    else if(m_type==5)
        sendImg();
    else if(m_type==6)
        sendPublishRes();
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
    if(m_status==0)
    {
        regisRes+="HTTP/1.1 200 OK\r\n";
        content = R"({
    "success": true,
    "message": "注册成功"
    })";
    }
    else
    {
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
    if(m_status)
    {
        std::string logRes,content;
        logRes = "HTTP/1.1 401 Unauthorized\r\n";
        content = "{\"code\":401,\"msg\":\"failed\"}";
        logRes+="Content-Type: application/json; charset=utf-8\r\n";
        logRes+="Content-Length: "+std::to_string(content.size())+"\r\n\r\n";
        logRes+=content;
        send(m_fd,logRes.c_str(),logRes.size(),0);
    }
    else
    {
        std::string midEdge;
    std::string html = R"(<!DOCTYPE html>
<html>
<body style="background-color: #e6f7ff; margin: 0; min-height: 100vh; display: flex; justify-content: center; align-items: center;">
    <div style="text-align: center; font-size: 18px; color: #333;">
        登录成功，正在跳转...
    </div>

<script>
const userId = ')"+m_msg+R"(';

localStorage.setItem('uid', userId);
  
setTimeout(() => {
  const encodedId = encodeURIComponent(userId);
  window.location.href = `/home?id=${encodedId}`;
}, 1000);
</script>
</body>
</html>)";
    midEdge += "HTTP/1.1 200 OK\r\n";
    midEdge += "Content-Type: text/html; charset=utf-8\r\n";
    midEdge += "Content-Length: " + std::to_string(html.size()) + "\r\n\r\n";
    midEdge += html;

    send(m_fd, midEdge.c_str(), midEdge.size(), 0);
    }
}

void writeTask::sendMainHtml()
{
    std::ifstream ifs("mainedge.html", std::ios::binary);
    //将整个文件内容读入到string中
    std::string htmlcontent(std::istreambuf_iterator<char>(ifs),{});//两个参数分别为创建迭代器，默认迭代器
    ifs.close(); 
    std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "+
        std::to_string(htmlcontent.size())+"\r\n\r\n"+htmlcontent;
    send(m_fd,msg.c_str(), msg.size(), 0);
}

void writeTask::sendFindMsg()
{
    std::string httpmsg = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: ";
    httpmsg+=std::to_string(m_msg.size());
    httpmsg+="\r\n\r\n";
    httpmsg+=m_msg;


    send(m_fd,httpmsg.c_str(),httpmsg.size(),0);
}

void writeTask::sendImg()
{
    std::string path = "./img"+m_msg;//注意相对路径

    std::ifstream file(path, std::ios::binary);  // 二进制方式读取图片
    // 读取文件内容到字符串
    std::string content((std::istreambuf_iterator<char>(file)), 
                        std::istreambuf_iterator<char>());
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: image/png\r\n";
    response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
    response += "\r\n";
    response += content;
    send(m_fd, response.c_str(), response.size(), 0);
}

void writeTask::sendPublishRes()
{
    std::string pubRes,content;
    if(m_status)
    {
        pubRes = "HTTP/1.1 400 Bad Request\r\n";
        content = R"({
    "success": false,
    "message": "发布失败",
    })";
        pubRes+="Content-Type: application/json; charset=utf-8\r\n";  
    }
    else
    {
        pubRes = "HTTP/1.1 200 OK\r\n";
        content = R"({
  "success": true,
  "message": "发布成功"
    })";
        pubRes+="Content-Type: application/json; charset=utf-8\r\n";
    }
    pubRes+="Content-Length: "+std::to_string(content.size())+"\r\n\r\n";
    pubRes+=content;
    send(m_fd,pubRes.c_str(),pubRes.size(),0);
}
