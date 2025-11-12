#include "tools.h"

#include<fcntl.h>

//设置非阻塞
void setNonBlock(int fd)
{
    int flag = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,flag|FNONBLOCK);
}

//添加epoll监听
void addWaitFd(int epoll, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    epoll_ctl(epoll,EPOLL_CTL_ADD,fd,&event);
}

//删除epoll监听
void deletWaitFd(int epoll, int fd)
{
    epoll_ctl(epoll,EPOLL_CTL_DEL,fd,nullptr);
}

//将16进制字符转化为数字
int hexCharToInt(char c)
{
    if(c<='9'&&c>='0')return c-'0';
    else if(c<='Z'&&c>='A')return 10+c-'A';
    else if(c<='z'&&c>='a')return 10+c-'a';
    return -1;
}

//url解码
//主要是把utf8编码的数据转化为中文字符
std::string urlDecode(std::string &encode)
{
    std::string decode;
    int n = encode.size();
    for(int i = 0;i<n;)
    {
        if(encode[i]=='%'&&i+2<n)
        {
            int a = hexCharToInt(encode[i+1]);
            int b = hexCharToInt(encode[i+2]);
            if(a!=-1&&b!=-1)
            {
                char byte = (a<<4)|b;
                decode+=byte;
                i+=3;
            }
            else
            {
                decode+=encode[i];
                i++;
            }
        }
        else
        {
            decode+=encode[i];
            i++;
        }
    }
    return decode;
}
