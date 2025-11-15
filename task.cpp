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

// 临时编译指令g++ -std=c++11 *.o task.cpp -o text -pthread -lmysqlclient

void ListenTask::process()
{
    int clientfd = accept(m_fd, nullptr, nullptr);
    setNonBlock(clientfd);
    addWaitFd(m_epoll, clientfd);
    delete this;
}

void readTask::process()
{
    // char buf[2048]{};
    // std::string recvMsg;
    // int totlen = 0;
    // int curlen = 0;
    // bool head = false;
 
    // while (1)//图片等大数据报文要循环接收才能完整
    // {
    //     int ret = recv(m_fd, buf, sizeof(buf), 0);
    //     if (ret < 0)
    //     {
    //         if (errno == EAGAIN || errno == EWOULDBLOCK)
    //         {
    //             // 非阻塞状态，暂时无数据到达
    //             addWaitFd(m_epoll, m_fd);
    //             return;
    //         }
    //         deletWaitFd(m_epoll, m_fd);
    //         close(m_fd);
    //         delete this;
    //         return;
    //     }
    //     else if (ret == 0)
    //     {
    //         // 客户端主动断开连接
    //         deletWaitFd(m_epoll, m_fd);
    //         close(m_fd);
    //         delete this;
    //         return;
    //     }
    //     // std::cout<<buf<<std::endl;
    //     recvMsg.append(buf,ret);
    //     curlen+=ret;
    //     if (!head)
    //     {
    //         int index = recvMsg.find("Content-Length: ");
    //         if(index!=std::string::npos)
    //         {
    //             index+=16;
    //             int endpos = recvMsg.find("\r\n",index);
    //             totlen = std::stoi(recvMsg.substr(index,endpos-index));
    //         }
    //         else
    //         {
    //             totlen = curlen;
    //         }
    //          int ct_pos = recvMsg.find("Content-Type: multipart/form-data; boundary=");
    //         if (ct_pos != std::string::npos)
    //         {
    //             ct_pos += 44; // 跳过"Content-Type: multipart/form-data; boundary="
    //             int ct_end = recvMsg.find("\r\n", ct_pos);
    //             m_headers["boundary"] = recvMsg.substr(ct_pos, ct_end - ct_pos); // 提前设置boundary
    //             std::cout<< m_headers["boundary"] <<std::endl;
    //         }
    //         head = true;
    //     }
    //     if (recvMsg.find("multipart/form-data") != std::string::npos)
    //     {
    //         std::string boundary = m_headers["boundary"];
    //         std::string end_boundary = "--" + boundary + "--";
    //         if (recvMsg.find(end_boundary) != std::string::npos)
    //         {
    //             break; // 找到结束边界
    //         }
    //     }
        
    //     if (head && curlen >= totlen)
    //     {
    //         break; // 普通表单数据，根据 Content-Length 判断
    //     }
    // }

    // // if(recvMsg.size()<300)std::cout << "buf:" << recvMsg << std::endl;
    // // std::cout<<"buf:"<<recvMsg.substr(0,300)<<std::endl;
    // handle(recvMsg);
    // delete this;

    char buf[1024]{};
    std::string recvMsg;
    int totlen = 0;
    int curlen = 0;
    bool head = false;
    
    while (1)
    {
        int ret = recv(m_fd, buf, sizeof(buf), 0);
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 非阻塞状态，暂时无数据到达
                addWaitFd(m_epoll, m_fd);
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
        
        recvMsg.append(buf, ret);
        curlen += ret;
        
        if (!head)
        {
            int index = recvMsg.find("Content-Length: ");
            if(index != std::string::npos)
            {
                index += 16;
                int endpos = recvMsg.find("\r\n", index);
                if (endpos != std::string::npos)
                {
                    totlen = std::stoi(recvMsg.substr(index, endpos-index));
                    std::cout << "检测到Content-Length: " << totlen << std::endl;
                    head = true;
                }
            }
            else
            {
                // 检查是否已经收到完整头部（没有消息体的请求）
                int header_end = recvMsg.find("\r\n\r\n");
                if (header_end != std::string::npos)
                {
                    // 头部结束，没有消息体，直接处理
                    break;
                }
            }
        }
        
        if(head && curlen >= totlen)
        {
            break;
        }
        
        memset(buf, 0, sizeof(buf));
    }
    
    std::cout << "成功接收数据，长度: " << recvMsg.size() << std::endl;
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
    // size_t pos = 0;
    // // 拆分请求行
    // size_t index = recvMsg.find("\r\n", 0);
    // std::string request_line = recvMsg.substr(0, index);
    // std::istringstream iss(request_line);
    // iss >> m_method;
    // iss >> m_path;
    // iss >> m_version; // 该变量在该项目中没有作用
    // while (1)
    // {
    //     pos = index + 2;
    //     index = recvMsg.find("\r\n", pos);
    //     if (index == pos)
    //         break; // 检测到空行退出
    //     std::string tmp = recvMsg.substr(pos, index - pos);
    //     // std::string bound = "Content-Type: multipart/form-data; boundary=";
    //     // if(tmp.substr(0,bound.size())==bound)
    //     // {
    //     //     m_headers["boundary"] = tmp.substr(bound.size());
    //     // }
    //     size_t colon = tmp.find(": ", 0); // 查找冒号
    //     std::string key = tmp.substr(0, colon);
    //     std::string value = tmp.substr(colon + 2);
    //     m_headers[key] = value;
    // }
    // pos += 2;
    // if (pos < recvMsg.size())
    // {
    //     m_body = recvMsg.substr(pos);
    // }
    // // TODO:后续任务分发
    // if (m_method == "GET")
    // {
    //     if (m_path == "/") // 检测到首页面的get请求
    //     {
    //         Task *task = new writeTask(m_epoll, m_fd, 0, 0, "");
    //         ThreadPool::addTask(task);
    //     }
    //     else if (m_path.substr(0, 5) == "/home")
    //     {
    //         Task *task = new writeTask(m_epoll, m_fd, 3, 0, "");
    //         ThreadPool::addTask(task);
    //     }
    //     else if (m_path.substr(0, 4) == "/img")
    //     {
    //         Task *task = new writeTask(m_epoll, m_fd, 5, 0, m_path.substr(4));
    //         ThreadPool::addTask(task);
    //     }
    // }
    // else if (m_method == "POST")
    // {
    //     if (m_path == "/api/login") // 登录请求
    //     {
    //         userLogin();
    //     }
    //     else if (m_path == "/api/register") // 注册请求
    //     {
    //         userRegister();
    //     }
    //     else if (m_path == "/api/find")
    //     {
    //         findMsg();
    //     }
    //     else if (m_path == "/api/attention")
    //     {
    //         attentionMsg();
    //     }
    //     else if (m_path == "/api/mypost")
    //     {
    //         myMsg();
    //     }
    //     else if (m_path == "/api/publish")
    //     {
    //         publish();
    //     }
    //     else if (m_path == "/api/like")
    //     {
    //         addLike();
    //     }
    //     else if (m_path == "/api/unlike")
    //     {
    //         delLike();
    //     }
    //     else if (m_path == "/api/getcomment")
    //     {
    //         getComment();
    //     }
    //     else if (m_path == "/api/postcomment")
    //     {
    //         postComment();
    //     }
    //     else if (m_path == "/api/profilepage")
    //     {
    //         profilePage();
    //     }
    //     else if (m_path == "/api/changemsg")
    //     {
    //         changemsg();
    //     }
    //     else if (m_path == "/api/uploadavatar")
    //     {
    //         std::cout<<"进入上传程序"<<std::endl;
    //         uploadavatar();
    //     }
    //     else if (m_path == "/api/follow")
    //     {
    //         setFollow();
    //     }
    //     else if (m_path == "/api/unfollow")
    //     {
    //         delFollow();
    //     }
    // }
     // 当前位置
    size_t pos = 0;
    // 拆分请求行
    size_t index = recvMsg.find("\r\n", 0);
    std::string request_line = recvMsg.substr(0, index);
    std::istringstream iss(request_line);
    iss >> m_method;
    iss >> m_path;
    iss >> m_version;
    
    std::cout << "处理请求: " << m_method << " " << m_path << std::endl;
    
    // 解析头部
    while (1)
    {
        pos = index + 2;
        index = recvMsg.find("\r\n", pos);
        if (index == pos)
            break; // 检测到空行退出
        std::string tmp = recvMsg.substr(pos, index - pos);
        std::string bound = "Content-Type: multipart/form-data; boundary=";
        if(tmp.substr(0,bound.size())==bound)
        {
            m_headers["boundary"] = tmp.substr(bound.size());
        }
        size_t colon = tmp.find(": ", 0);
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
    if (m_method == "GET")
    {
        if (m_path == "/")
        {
            Task *task = new writeTask(m_epoll, m_fd, 0, 0, "");
            ThreadPool::addTask(task);
        }
        else if (m_path.substr(0, 5) == "/home")
        {
            Task *task = new writeTask(m_epoll, m_fd, 3, 0, "");
            ThreadPool::addTask(task);
        }
        else if (m_path.substr(0, 4) == "/img")
        {
            Task *task = new writeTask(m_epoll, m_fd, 5, 0, m_path.substr(4));
            ThreadPool::addTask(task);
        }
    }
    else if (m_method == "POST")
    {
        if (m_path == "/api/login")
        {
            std::cout << "处理登录请求" << std::endl;
            userLogin();
        }
        else if (m_path == "/api/register")
        {
            userRegister();
        }
        else if (m_path == "/api/find")
        {
            findMsg();
        }
        else if (m_path == "/api/attention")
        {
            attentionMsg();
        }
        else if (m_path == "/api/mypost")
        {
            myMsg();
        }
        else if (m_path == "/api/publish")
        {
            publish();
        }
        else if (m_path == "/api/like")
        {
            addLike();
        }
        else if (m_path == "/api/unlike")
        {
            delLike();
        }
        else if (m_path == "/api/getcomment")
        {
            getComment();
        }
        else if (m_path == "/api/postcomment")
        {
            postComment();
        }
        else if (m_path == "/api/profilepage")
        {
            profilePage();
        }
        else if (m_path == "/api/changemsg")
        {
            changemsg();
        }
        else if (m_path == "/api/uploadavatar")
        {
            std::cout << "=== 处理头像上传请求 ===" << std::endl;
            uploadavatar();
        }
        else if (m_path == "/api/follow")
        {
            setFollow();
        }
        else if (m_path == "/api/unfollow")
        {
            delFollow();
        }
    }
}

void readTask::userLogin()
{
    // user=111&password=aaa
    // 这里的user实际上还可能是email
    std::string user, pwd;
    int pindex = m_body.find("&password=");
    user = m_body.substr(5, pindex - 5);
    user = urlDecode(user);
    pwd = m_body.substr(pindex + 10);
    std::string selsql = "SELECT id,username,headimg FROM `User` WHERE username = '" + user + "' AND password = '" + pwd + "';";
    auto result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if (!result.empty() && !result["id"].empty()) // 查询到用户名密码相符
    {
        std::string _id(result["id"][0]);
        std::string _name(result["username"][0]);
        std::string _img(result["headimg"][0]);
        Task *task = new writeTask(m_epoll, m_fd, 2, 0, _id + " " + _name + " " + _img);
        ThreadPool::addTask(task);
        return;
    }
    selsql = "SELECT id,username,headimg FROM `User` WHERE email = '" + user + "' AND password = '" + pwd + "';";
    result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if (!result.empty() && !result["id"].empty()) // 查询到邮箱密码相符
    {
        std::string _id(result["id"][0]);
        std::string _name(result["username"][0]);
        std::string _img(result["headimg"][0]);
        Task *task = new writeTask(m_epoll, m_fd, 2, 0, _id + " " + _name + " " + _img);
        ThreadPool::addTask(task);
        return;
    }
    // 无相符内容
    Task *task = new writeTask(m_epoll, m_fd, 2, 1, "");
    ThreadPool::addTask(task);
}

void readTask::userRegister()
{
    // user=111&email=ttt.com&password=aaa
    std::string user, email, pwd;
    int eindex = m_body.find("&email=");
    user = m_body.substr(5, eindex - 5);
    user = urlDecode(user);
    int pindex = m_body.find("&password=");
    email = m_body.substr(eindex + 7, pindex - eindex - 7);
    pwd = m_body.substr(pindex + 10);
    std::string selsql = "SELECT id FROM `User` WHERE username = '" + user + "' OR email = '" + email + "';";
    auto result = DataBase::getInstance()->executeSQL(selsql.c_str());
    if (!result.empty() && !result["id"].empty())
    {
        Task *task = new writeTask(m_epoll, m_fd, 1, 1, "");
        ThreadPool::addTask(task);
        return;
    }
    // 下面的语句存在sql注入风险，但是只是作为练手项目，暂不考虑这些问题
    std::string inssql = "INSERT INTO `User` (username, password, email,personality) VALUES ('" + user + "', '" + pwd + "', '" + email + "','');";
    DataBase::getInstance()->executeSQL(inssql.c_str());
    Task *task = new writeTask(m_epoll, m_fd, 1, 0, "");
    ThreadPool::addTask(task);
}

// 以下这些返回的json数据代码中都缺少特殊字符的处理，目前只是为了实现功能，暂不处理
void readTask::findMsg()
{
    // page=...&id=...
    int index = m_body.find("&id=");
    std::string page = m_body.substr(5, index - 5);
    std::string id = m_body.substr(index + 4);

    int npage = std::stoi(page);
    // 查询消息总数量
    std::string sql = "select count(*) as cnt from Message;";
    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string s(res["cnt"][0]);
    int cnt = std::stoi(s);
    // 向上取整分页
    int allpage = cnt / 3 + (cnt % 3 != 0);
    // 每三组数据分为一组，查询第page组数据
    sql = "SELECT username,headimg,txt,likes,comments,m.id,CASE WHEN l.msgid IS NOT NULL THEN 1 ELSE 0 END AS isliked FROM `Message` m LEFT JOIN `User` u ON m.authorid = u.id LEFT JOIN `Likes` l ON m.id = l.msgid AND l.userid = " + id + " ORDER BY m.id DESC LIMIT 3 OFFSET " + std::to_string((npage - 1) * 3) + ";";
    res = DataBase::getInstance()->executeSQL(sql.c_str());

    std::string backMsg = "{\n\"data\":[\n";
    // 返回数据示例
    //     {
    //      "data": [
    //      {
    //           "author": "测试",
    //          "avatar": "http://192.168.88.101:19200/img/test.png",
    //          "content": "周末去了这家餐厅，味道超赞",
    //          "likes": 76,
    //          "comments": 12,
    //          "msgid": 1001,
    //          "isLiked": false
    //      },
    //      {
    //          "author": "是一对J",
    //          "avatar": "http://192.168.88.101:19200/img/user1.png",
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
    for (int i = 0; i < n; i++)
    {
        backMsg += "{\n\"author\": \"";
        backMsg += res["username"][i];
        backMsg += "\",\n";
        backMsg += "\"avatar\": \"";
        backMsg += "http://192.168.88.101:19200"; // 需更换为实际服务器ip
        backMsg += res["headimg"][i];
        backMsg += "\",\n";
        backMsg += "\"content\": \"";
        backMsg += res["txt"][i];
        backMsg += "\",\n";
        backMsg += "\"likes\": ";
        backMsg += res["likes"][i];
        backMsg += ",\n";
        backMsg += "\"comments\": ";
        backMsg += res["comments"][i];
        backMsg += ",\n";
        backMsg += "\"msgid\": ";
        backMsg += res["id"][i];
        backMsg += ",\n";
        backMsg += "\"isLiked\": ";
        backMsg += res["isliked"][i];
        backMsg += "\n";
        backMsg += "}";
        if (i != n - 1)
            backMsg += ",";
        backMsg += "\n";
    }
    backMsg += "],\n\"totalPages\": " + std::to_string(allpage) + "\n}";

    Task *task = new writeTask(m_epoll, m_fd, 4, 0, backMsg);
    ThreadPool::addTask(task);
}

void readTask::attentionMsg()
{
    size_t index = m_body.find("&id=");
    std::string page = m_body.substr(5, index - 5);
    std::string id = m_body.substr(index + 4);
    std::string sql = "select count(*) as cnt from Message m inner join Follows f on m.authorid = f.followed_id where f.follower_id = " + id + ";";
    int npage = std::stoi(page);
    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string s(res["cnt"][0]);
    int cnt = std::stoi(s);
    int allpage = cnt / 3 + (cnt % 3 != 0);
    sql = "SELECT u.username,u.headimg,m.txt,m.likes,m.comments,m.id,CASE WHEN l.msgid IS NOT NULL THEN 1 ELSE 0 END AS isliked FROM `Message` m INNER JOIN `User` u ON m.authorid = u.id INNER JOIN `Follows` f ON f.follower_id = " + id + " AND m.authorid = f.followed_id LEFT JOIN `Likes` l ON m.id = l.msgid AND l.userid = " + id + " ORDER BY m.id DESC LIMIT 3 OFFSET " + std::to_string((npage - 1) * 3) + ";";
    res = DataBase::getInstance()->executeSQL(sql.c_str());

    std::string backMsg = "{\n\"data\":[\n";

    int n = res["username"].size();
    for (int i = 0; i < n; i++)
    {
        backMsg += "{\n\"author\": \"";
        backMsg += res["username"][i];
        backMsg += "\",\n";
        backMsg += "\"avatar\": \"";
        backMsg += "http://192.168.88.101:19200"; // 需更换为实际服务器ip
        backMsg += res["headimg"][i];
        backMsg += "\",\n";
        backMsg += "\"content\": \"";
        backMsg += res["txt"][i];
        backMsg += "\",\n";
        backMsg += "\"likes\": ";
        backMsg += res["likes"][i];
        backMsg += ",\n";
        backMsg += "\"comments\": ";
        backMsg += res["comments"][i];
        backMsg += ",\n";
        backMsg += "\"msgid\": ";
        backMsg += res["id"][i];
        backMsg += ",\n";
        backMsg += "\"isLiked\": ";
        backMsg += res["isliked"][i];
        backMsg += "\n";
        backMsg += "}";
        if (i != n - 1)
            backMsg += ",";
        backMsg += "\n";
    }
    backMsg += "],\n\"totalPages\": " + std::to_string(allpage) + "\n}";

    Task *task = new writeTask(m_epoll, m_fd, 4, 0, backMsg);
    ThreadPool::addTask(task);
}

void readTask::myMsg()
{
    size_t index = m_body.find("&id=");
    std::string page = m_body.substr(5, index - 5);
    std::string id = m_body.substr(index + 4);
    std::string sql = "select count(*) as cnt from Message where authorid = " + id + ";";
    int npage = std::stoi(page);
    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string s(res["cnt"][0]);
    int cnt = std::stoi(s);
    int allpage = cnt / 3 + (cnt % 3 != 0);
    sql = "SELECT username,headimg,txt,likes,comments,m.id,CASE WHEN l.msgid IS NOT NULL THEN 1 ELSE 0 END AS isliked FROM `Message` m LEFT JOIN `User` u ON m.authorid = u.id LEFT JOIN `Likes` l ON m.id = l.msgid AND l.userid = " + id + " WHERE m.authorid = " + id + " ORDER BY m.id DESC LIMIT 3 OFFSET " + std::to_string((npage - 1) * 3) + ";";
    res = DataBase::getInstance()->executeSQL(sql.c_str());

    std::string backMsg = "{\n\"data\":[\n";

    int n = res["username"].size();
    for (int i = 0; i < n; i++)
    {
        backMsg += "{\n\"author\": \"";
        backMsg += res["username"][i];
        backMsg += "\",\n";
        backMsg += "\"avatar\": \"";
        backMsg += "http://192.168.88.101:19200"; // 需更换为实际服务器ip
        backMsg += res["headimg"][i];
        backMsg += "\",\n";
        backMsg += "\"content\": \"";
        backMsg += res["txt"][i];
        backMsg += "\",\n";
        backMsg += "\"likes\": ";
        backMsg += res["likes"][i];
        backMsg += ",\n";
        backMsg += "\"comments\": ";
        backMsg += res["comments"][i];
        backMsg += ",\n";
        backMsg += "\"msgid\": ";
        backMsg += res["id"][i];
        backMsg += ",\n";
        backMsg += "\"isLiked\": ";
        backMsg += res["isliked"][i];
        backMsg += "\n";
        backMsg += "}";
        if (i != n - 1)
            backMsg += ",";
        backMsg += "\n";
    }
    backMsg += "],\n\"totalPages\": " + std::to_string(allpage) + "\n}";

    Task *task = new writeTask(m_epoll, m_fd, 4, 0, backMsg);
    ThreadPool::addTask(task);
}

void readTask::publish()
{
    // id=...&text=...
    size_t index = m_body.find("&text=");
    std::string id = m_body.substr(3, index - 3);
    std::string text = m_body.substr(index + 6);
    text = urlDecode(text);
    if (text.empty())
    {
        Task *task = new writeTask(m_epoll, m_fd, 6, 1, "");
        ThreadPool::addTask(task);
        return;
    }
    std::string sql = "INSERT INTO `Message` (`txt`, `authorid`) VALUES ('" + text + "', " + id + ");";

    DataBase::getInstance()->executeSQL(sql.c_str());
    Task *task = new writeTask(m_epoll, m_fd, 6, 0, "");
    ThreadPool::addTask(task);
}

void readTask::addLike()
{
    // userid=...&msgid=...
    size_t index = m_body.find("&msgid=");
    std::string userid = m_body.substr(7, index - 7);
    std::string msgid = m_body.substr(index + 7);
    std::string sql = "INSERT INTO `Likes` (`msgid`, `userid`) VALUES (" + msgid + ", " + userid + ");";

    DataBase::getInstance()->executeSQL(sql.c_str());

    sql = "UPDATE `Message` SET `likes` = `likes` + 1 WHERE `id` = " + msgid + ";";

    DataBase::getInstance()->executeSQL(sql.c_str());
}

void readTask::delLike()
{
    // userid=...&msgid=...
    size_t index = m_body.find("&msgid=");
    std::string userid = m_body.substr(7, index - 7);
    std::string msgid = m_body.substr(index + 7);

    std::string sql = "DELETE FROM `Likes` WHERE `msgid` = " + msgid + " AND `userid` = " + userid + ";";

    DataBase::getInstance()->executeSQL(sql.c_str());

    sql = "UPDATE `Message` SET `likes` = `likes` - 1 WHERE `id` = " + msgid + ";";

    DataBase::getInstance()->executeSQL(sql.c_str());
}

void readTask::getComment()
{
    // msgid=...&page=
    size_t index = m_body.find("&page=");
    std::string msgid = m_body.substr(6, index - 6);
    std::string page = m_body.substr(index + 6);
    int npage = std::stoi(page);

    std::string sql = "select count(*) as cnt from Comments where msgid = " + msgid;

    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string s(res["cnt"][0]);
    int cnt = std::stoi(s);
    // 向上取整分页
    int allpage = cnt / 3 + (cnt % 3 != 0);

    sql = "select username,headimg,content from Comments c left join `User` u on c.userid = u.id WHERE msgid = " + msgid + "  ORDER BY c.id DESC LIMIT 3 OFFSET " + std::to_string((npage - 1) * 3) + ";";
    res = DataBase::getInstance()->executeSQL(sql.c_str());
    std::string backMsg = "{\n\"totalPages\": " + std::to_string(allpage) + ",\n\"data\": [\n";
    // 示例返回内容
    //{
    //     "totalPages": 3,
    //     "data": [
    //      {
    //          "author": "张三",
    //          "avatar": "http://192.168.88.101:19200/img/user1.png",
    //          "content": "这个帖子很有道理！"
    //      },
    //     {
    //         "author": "李四",
    //          "avatar": "http://192.168.88.101:19200/img/user2.png",
    //          "content": "同意楼上观点~"
    //      },
    //      {
    //          "author": "王五",
    //          "avatar": "http://192.168.88.101:19200/img/user3.png",
    //          "content": "我补充一点..."
    //      }
    //      ]
    // }
    int n = res["username"].size();
    for (int i = 0; i < n; i++)
    {
        backMsg += "{\n\"author\": \"";
        backMsg += res["username"][i];
        backMsg += "\",\n";
        backMsg += "\"avatar\": \"";
        backMsg += "http://192.168.88.101:19200"; // 需更换为实际服务器ip
        backMsg += res["headimg"][i];
        backMsg += "\",\n";
        backMsg += "\"content\": \"";
        backMsg += res["content"][i];
        backMsg += "\"\n";
        backMsg += "}";
        if (i != n - 1)
            backMsg += ",";
        backMsg += "\n";
    }
    backMsg += "]\n}";

    Task *task = new writeTask(m_epoll, m_fd, 4, 0, backMsg);
    ThreadPool::addTask(task);
}

void readTask::postComment()
{
    // userid=...&msgid=...&txt=...
    size_t index = m_body.find("&msgid=");
    std::string userid = m_body.substr(7, index - 7);
    size_t pos = index + 7;
    index = m_body.find("&txt=");
    std::string msgid = m_body.substr(pos, index - pos);
    std::string txt = m_body.substr(index + 5);

    txt = urlDecode(txt);

    std::string sql = "INSERT INTO `Comments` (`msgid`, `userid`, `content`) VALUES (" + msgid + "," + userid + ", '" + txt + "');";
    DataBase::getInstance()->executeSQL(sql.c_str());
    sql = "UPDATE `Message` SET `comments` = `comments` + 1 WHERE `id` = " + msgid + ";";
    DataBase::getInstance()->executeSQL(sql.c_str());

    Task *task = new writeTask(m_epoll, m_fd, 6, 0, "");
    ThreadPool::addTask(task);
}

void readTask::profilePage()
{
    // username=...&userid=...
    size_t index = m_body.find("&userid=");
    std::string name = m_body.substr(9, index - 9);
    name = urlDecode(name);
    std::string id = m_body.substr(index + 8);
    std::string sql = "select headimg,personality,follows,fans,EXISTS( SELECT 1 FROM Follows f WHERE f.follower_id = " + id + " AND f.followed_id = u.id ) AS is_followed from `User` u where username = '" + name + "';";
    auto res = DataBase::getInstance()->executeSQL(sql.c_str());
    // 示例返回数据
    //     {
    //      "avatar": "http://192.168.88.101:19200/img/user1.png",
    //      "username": "张三",
    //      "personality": "热爱生活，喜欢分享日常",
    //      "followCount": 156,
    //      "fanCount": 328,
    //      "isFollowed": true
    //     }
    std::string backmsg = R"({
    "avatar": "http://192.168.88.101:19200)" +
                          std::string(res["headimg"][0]) + R"(",
    "username": ")" + name +
                          R"(",
    "personality": ")" + std::string(res["personality"][0]) +
                          R"(",
    "followCount": )" + std::string(res["follows"][0]) +
                          R"(,
    "fanCount": )" + std::string(res["fans"][0]) +
                          R"(,
    "isFollowed": )" + std::string(res["is_followed"][0]) +
                          R"(
})";


    Task *task = new writeTask(m_epoll, m_fd, 4, 0, backmsg);
    ThreadPool::addTask(task);
}

void readTask::changemsg()
{
    // id=...&name=...&personality=...
    size_t index = m_body.find("&name=");
    std::string id = m_body.substr(3, index - 3);
    size_t pos = index + 6;
    index = m_body.find("&personality=");
    std::string name = m_body.substr(pos, index - pos);
    name = urlDecode(name);
    std::string personality = m_body.substr(index + 13);
    personality = urlDecode(personality);

    std::string sql = "UPDATE User SET username = '" + name + "', personality = '" + personality + "' WHERE id = " + id + ";";
    DataBase::getInstance()->executeSQL(sql.c_str());

    Task *task = new writeTask(m_epoll, m_fd, 6, 0, "");
    ThreadPool::addTask(task);
}

//暂时只写png格式图片
void readTask::uploadavatar()
{
    //示例报文
//  POST /api/uploadavatar HTTP/1.1
//  Host: 192.168.88.101:19200
//  Connection: keep-alive
//  Content-Length: 12345  # 总数据长度（包含所有内容）
//  User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/142.0.0.0
//  Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryABC123XYZ
//  Accept: */*
//  Origin: http://192.168.88.101:19200
//  Referer: http://192.168.88.101:19200/home?id=2

    //以下是主体部分
//  ----WebKitFormBoundaryABC123XYZ
//  Content-Disposition: form-data; name="userid"

//  2
//  ----WebKitFormBoundaryABC123XYZ
//  Content-Disposition: form-data; name="avatar"; filename="test.png"
//  Content-Type: image/png

//  [图片二进制数据...]
//  ----WebKitFormBoundaryABC123XYZ--
    std::cout<<"len:"<<m_body.size()<<std::endl;
    size_t index = m_body.find("\r\n\r\n");//虽然说写法可能不怎么好，现在就先暂时这样写了
    size_t pos = index+4;
    index = m_body.find("\r\n",pos);
    std::string id = m_body.substr(pos,index-pos);
    std::cout<<"id:"<<id<<std::endl;
    index = m_body.find("\r\n\r\n",pos);
    pos = index+4;
    std::string bound = "\r\n--"+m_headers["boundary"]+"--";
    index = m_body.find(bound,pos);
    std::string img = m_body.substr(pos,index-pos);

    std::string filename = "/img/user"+id+".png";
    std::ofstream ofs("."+filename,std::ios::binary);
    if(!ofs.is_open())std::cout<<"打开失败"<<std::endl;
    ofs.write(img.data(),img.size());
    ofs.close();

    std::string sql = "UPDATE User SET headimg = '"+filename+"' WHERE id = "+id+";";
    DataBase::getInstance()->executeSQL(sql.c_str());

    Task *task = new writeTask(m_epoll, m_fd, 7, 0, filename);
    ThreadPool::addTask(task);
}

void readTask::setFollow()
{
    // myid=...&upname=...
    size_t index = m_body.find("&upname=");
    std::string myid = m_body.substr(5, index - 5);
    std::string upname = m_body.substr(index + 8);
    upname = urlDecode(upname);

    std::string sql = "INSERT INTO Follows (follower_id, followed_id) VALUES (" + myid + ",(SELECT id FROM User WHERE username = '" + upname + "'));";
    DataBase::getInstance()->executeSQL(sql.c_str());
    sql = "UPDATE User SET follows = follows + 1 WHERE id = " + myid + ";";
    DataBase::getInstance()->executeSQL(sql.c_str());
    sql = "UPDATE User SET fans = fans + 1 WHERE username = '" + upname + "';";
    DataBase::getInstance()->executeSQL(sql.c_str());
}

void readTask::delFollow()
{
    // myid=...&upname=...
    size_t index = m_body.find("&upname=");
    std::string myid = m_body.substr(5, index - 5);
    std::string upname = m_body.substr(index + 8);
    upname = urlDecode(upname);

    std::string sql = "DELETE FROM Follows WHERE follower_id = " + myid + " AND followed_id = (SELECT id FROM User WHERE username = '" + upname + "');";
    DataBase::getInstance()->executeSQL(sql.c_str());
    sql = "UPDATE User SET follows = follows - 1 WHERE id = " + myid + ";";
    DataBase::getInstance()->executeSQL(sql.c_str());
    sql = "UPDATE User SET fans = fans - 1 WHERE username = '" + upname + "';";
    DataBase::getInstance()->executeSQL(sql.c_str());
}

void writeTask::process()
{
    if (m_type == 0)
        sendLoginHtml();
    else if (m_type == 1)
        sendRegisRes();
    else if (m_type == 2)
        sendLogRes();
    else if (m_type == 3)
        sendMainHtml();
    else if (m_type == 4)
        sendBackMsg();
    else if (m_type == 5)
        sendImg();
    else if (m_type == 6)
        sendBackRes();
    else if(m_type==7)
        sendImgRes();
    delete this;
}

void writeTask::sendLoginHtml()
{
    std::ifstream ifs("entry.html", std::ios::binary);
    // 将整个文件内容读入到string中
    std::string htmlcontent(std::istreambuf_iterator<char>(ifs), {}); // 两个参数分别为创建迭代器，默认迭代器
    ifs.close();
    std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " +
                      std::to_string(htmlcontent.size()) + "\r\n\r\n" + htmlcontent;
    send(m_fd, msg.c_str(), msg.size(), 0);
}

void writeTask::sendRegisRes()
{
    std::string regisRes, content;
    if (m_status == 0)
    {
        regisRes += "HTTP/1.1 200 OK\r\n";
        content = R"({
    "success": true,
    "message": "注册成功"
    })";
    }
    else
    {
        regisRes += "HTTP/1.1 400 Bad Request\r\n";
        content = R"({
  "success": false,
  "message": "用户名已存在"
    })";
    }
    regisRes += "Content-Type: application/json; charset=utf-8\r\n";
    regisRes += "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";
    regisRes += content;
    send(m_fd, regisRes.c_str(), regisRes.size(), 0);
}

void writeTask::sendLogRes()
{
    if (m_status)
    {
        std::string logRes, content;
        logRes = "HTTP/1.1 401 Unauthorized\r\n";
        content = "{\"code\":401,\"msg\":\"failed\"}";
        logRes += "Content-Type: application/json; charset=utf-8\r\n";
        logRes += "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";
        logRes += content;
        send(m_fd, logRes.c_str(), logRes.size(), 0);
    }
    else
    {
        std::string midEdge;
        std::istringstream iss(m_msg);
        std::string userid, username, headimg;
        iss >> userid;
        iss >> username;
        iss >> headimg;
        std::string html = R"(<!DOCTYPE html>
<html>
<body style="background-color: #e6f7ff; margin: 0; min-height: 100vh; display: flex; justify-content: center; align-items: center;">
    <div style="text-align: center; font-size: 18px; color: #333;">
        登录成功，正在跳转...
    </div>

<script>
const userId = ')" + userid +
                           R"(';
const username = ')" + username +
                           R"(';
const avatarUrl = 'http://192.168.88.101:19200)" +
                           headimg + R"(';

localStorage.setItem('uid', userId);
localStorage.setItem('username', username);
localStorage.setItem('avatar', avatarUrl);
  
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
    // 将整个文件内容读入到string中
    std::string htmlcontent(std::istreambuf_iterator<char>(ifs), {}); // 两个参数分别为创建迭代器，默认迭代器
    ifs.close();
    std::string msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " +
                      std::to_string(htmlcontent.size()) + "\r\n\r\n" + htmlcontent;
    send(m_fd, msg.c_str(), msg.size(), 0);
}

void writeTask::sendBackMsg()
{
    std::string httpmsg = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: ";
    httpmsg += std::to_string(m_msg.size());
    httpmsg += "\r\n\r\n";
    httpmsg += m_msg;

    send(m_fd, httpmsg.c_str(), httpmsg.size(), 0);
}

void writeTask::sendImg()
{
    std::string path = "./img" + m_msg; // 注意相对路径

    std::ifstream file(path, std::ios::binary); // 二进制方式读取图片
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

void writeTask::sendBackRes()
{
    std::string pubRes, content;
    if (m_status)
    {
        pubRes = "HTTP/1.1 400 Bad Request\r\n";
        content = R"({
    "success": false,
    "message": "操作失败",
    })";
        pubRes += "Content-Type: application/json; charset=utf-8\r\n";
    }
    else
    {
        pubRes = "HTTP/1.1 200 OK\r\n";
        content = R"({
  "success": true,
  "message": "操作成功"
    })";
        pubRes += "Content-Type: application/json; charset=utf-8\r\n";
    }
    pubRes += "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";
    pubRes += content;
    send(m_fd, pubRes.c_str(), pubRes.size(), 0);
}

void writeTask::sendImgRes()
{
    std::string res,content;
    res = "HTTP/1.1 200 OK\r\n";
    res += "Content-Type: application/json; charset=utf-8\r\n";
    content = R"({
  "status": 200,
  "message": "头像上传成功",
  "data": {
    "headimg": ")"+m_msg+R"("
  }
})";
    res += "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";
    res += content;
    send(m_fd,res.c_str(),res.size(),0);
}
