#pragma once

#include<mysql/mysql.h>
#include<mutex>
#include<unordered_map>
#include<queue>
#include<vector>
#include<utility>

//使用单例操作模式的数据库类
//当连接池有空闲sql对象时使用该对象，无空闲对象时创建新对象
class DataBase
{
public:
    ~DataBase();
    static DataBase* getInstance();
    void init(const char* host,const char* user,const char* pwd,const char* databasename,
        unsigned int port = 0,const char* serversocket = nullptr,unsigned long flag = 0);
    std::unordered_map<std::string,std::vector<const char*>> executeSQL(const char* sql);
private:
    DataBase(){};
    DataBase(DataBase& oth) = delete;
    DataBase(DataBase&& oth) = delete;
    DataBase& operator=(DataBase&& oth) = delete;

    MYSQL* createSQL();
    MYSQL* getSQL();
private:
    std::queue<MYSQL*> m_sqppool;//数据库连接池
    const char* m_host;
    const char* m_user;
    const char* m_pwd;
    const char* m_databasename;
    unsigned int m_port;
    const char* m_socket;
    unsigned long m_clientFlag;
    unsigned int m_conn_cnt;
    static DataBase* m_database;
    static std::mutex m_obj_mutex;
    static std::mutex m_pool_mutex;
};

