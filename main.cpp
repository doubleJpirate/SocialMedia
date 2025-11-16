#include"webserver.h"
#include"threadpool.h"
#include"database.h"

int main()
{
// 	MYSQL* conn = mysql_init(nullptr);
// 	mysql_real_connect(conn,"localhost","root","123456","SocialMedia",0,nullptr,0);
// 	mysql_query(conn,"INSERT INTO `User` (username, password, email) VALUES ('lisi', 'lisi123', 'lisi@example.com');");
// 	MYSQL_RES* res = mysql_store_result(conn);

	ThreadPool::init(4);//初始化线程池
	DataBase::getInstance()->init("localhost","root","123456","SocialMedia",0,nullptr,0);
	WebServer* web = new WebServer();
	web->init(19200);//初始化通信套接字和epoll
	web->start();//设置epoll监听，网络连接开始
	return 0;
}
