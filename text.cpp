#include"webserver.h"
#include"threadpool.h"
#include"database.h"

int main()
{
	ThreadPool::init(4);//初始化线程池

	WebServer* web = new WebServer();
	web->init(19200);//初始化通信套接字和epoll
	web->start();//设置epoll监听，网络连接开始
	return 0;
}
