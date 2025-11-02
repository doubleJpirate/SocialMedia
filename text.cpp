#include<iostream>

#include"threadpool.h"

int main()
{
	std::cout<<"中文输出测试"<<std::endl;
	ThreadPool t(10);
	return 0;
}
