#include "threadpool.h"

#include<iostream>
#include<chrono>


int ThreadPool::p_threadnum = 0;
std::vector<std::thread> ThreadPool::p_workers;
std::queue<Task*> ThreadPool::p_tasks;
std::mutex ThreadPool::p_mutex;
std::condition_variable ThreadPool::p_condition;
std::atomic<bool> ThreadPool::p_stop(false);

void ThreadPool::init(int threadnum)
{
    p_threadnum = threadnum;
    if(threadnum<=0)
        throw std::invalid_argument("线程数量必须为正数");
    for(int i = 0;i<threadnum;i++)
    {
        p_workers.emplace_back(&ThreadPool::work,i+1);//绑定工作函数
        std::this_thread::sleep_for(std::chrono::microseconds(1000));//确保线程启动
    }
}

void ThreadPool::addTask(Task *task)
{
    std::lock_guard<std::mutex> lock(p_mutex);
    p_tasks.push(task);

    p_condition.notify_one();
}

void ThreadPool::work(int id)
{
    while(!p_stop)
    {
        Task* curTask = nullptr;
        std::unique_lock<std::mutex> lock(p_mutex);
        p_condition.wait(lock,[&](){
            return p_stop || !p_tasks.empty();//当线程池未停止运行且任务队列为空时阻塞函数
        });
        if(p_stop&&p_tasks.empty())break;//当线程池停止且任务队列为空时直接退出函数
        curTask = p_tasks.front();
        p_tasks.pop();
        lock.unlock();
        if(curTask==nullptr)break;
        curTask->process();
    }
}

void ThreadPool::stop()
{
    p_stop = true;
    p_condition.notify_all();
    for(int i = 0;i<p_threadnum;i++)
    {
        if(p_workers[i].joinable())
        {
            p_workers[i].join();
        }
    }
}
