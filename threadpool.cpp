#include "threadpool.h"

#include<iostream>

ThreadPool::ThreadPool(int threadnum):m_threadnum(threadnum),m_stop(false)
{
    if(threadnum<=0)
        throw std::invalid_argument("线程数量必须为正数");
    for(int i = 0;i<threadnum;i++)
    {
        m_workers.emplace_back(&ThreadPool::work,this,i+1);//绑定工作函数
        std::this_thread::sleep_for(std::chrono::microseconds(1000));//确保线程启动
    }
}

void ThreadPool::work(int id)
{
    std::cout<<"线程"<<id<<"已就绪"<<std::endl;
    while(!m_stop)
    {
        int curTask;//暂时用int替代
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock,[this](){
            return m_stop || !m_tasks.empty();//当线程池未停止运行且任务队列为空时阻塞函数
        });
        if(m_stop&&m_tasks.empty())break;//当线程池停止且任务队列为空时直接退出函数
        curTask = m_tasks.front();
        m_tasks.pop();
        lock.unlock();
        std::cout<<"线程"<<id<<"正在处理任务"<<std::endl;
        //TODO:完成任务主函数
        std::cout<<"线程"<<id<<"已退出"<<std::endl;
    }
}

int ThreadPool::addTask(int task)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.push(task);

    m_condition.notify_one();
    return 0;
}

ThreadPool::~ThreadPool()
{
    m_stop = true;
    m_condition.notify_all();
    for(int i = 0;i<m_threadnum;i++)
    {
        if(m_workers[i].joinable())
        {
            m_workers[i].join();
        }
    }
}
