#pragma once

#include<thread>
#include<vector>
#include<queue>
#include<functional>
#include<condition_variable>
#include<mutex>
#include<atomic>

#include"task.h"

//经典生产者消费者模型的线程池
//用命名空间代替类来实现，让多个文件访问同一个线程池
namespace ThreadPool
{
    void init(int num);
    void addTask(Task* task);
    void work(int id);
    void stop();

    extern int p_threadnum;
    extern std::vector<std::thread> p_workers;
    extern std::queue<Task*> p_tasks;
    extern std::mutex p_mutex;
    extern std::condition_variable p_condition;
    extern std::atomic<bool> p_stop;
};