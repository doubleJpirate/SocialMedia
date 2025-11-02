#pragma once

#include<thread>
#include<vector>
#include<queue>
#include<functional>
#include<condition_variable>
#include<mutex>
#include<atomic>

//经典生产者消费者模型的线程池
class ThreadPool{
public:
    ThreadPool(int threadnum);
    void work(int id);
    int addTask(int task);//暂时用int替代
    ~ThreadPool();
private:
    ThreadPool(){};
private:
    int m_threadnum;
    std::vector<std::thread> m_workers;
    std::queue<int> m_tasks;//暂时用int代替任务
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop;
};