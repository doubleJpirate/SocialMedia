#include"webserver.h"
#include"threadpool.h"

int main()
{
	ThreadPool::init(4);
	WebServer* web = new WebServer();
	web->init();
	web->start();
	return 0;
}
