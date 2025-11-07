#include "database.h"

std::mutex DataBase::m_obj_mutex;
std::mutex DataBase::m_pool_mutex;
DataBase* DataBase::m_database = nullptr;

DataBase::~DataBase()
{
    while(!m_sqppool.empty()){
        std::lock_guard<std::mutex> lock(m_pool_mutex);
        m_sqppool.pop();
        m_conn_cnt--;
    }
}

DataBase *DataBase::getInstance()
{
    if(m_database==nullptr){
        std::lock_guard<std::mutex> lock(m_obj_mutex);
        if(m_database = nullptr){
            m_database = new DataBase();
        }
    }
    return m_database;
}

void DataBase::init(const char *host, const char *user, const char *pwd, const char *databasename,
     unsigned int port, const char *serversocket, unsigned long flag)
{
    m_host = host;
    m_user = user;
    m_pwd = pwd;
    m_databasename = databasename;
    m_port = port;
    m_socket = serversocket;
    m_clientFlag = flag;
}

std::unordered_map<std::string, std::vector<const char *>> DataBase::executeSQL(const char *sql)
{
    MYSQL* conn = getSQL();
    std::unordered_map<std::string, std::vector<const char *>> ans;
    mysql_query(conn,sql);
    MYSQL_RES* res = mysql_store_result(conn);
    if(res!=nullptr)//是查询语句
    {
        MYSQL_FIELD* field;
        while(field = mysql_fetch_field(res))
        {
            ans[field->name] = std::vector<const char *>();
        }
        MYSQL_ROW row;
        while(row = mysql_fetch_row(res))
        {
            size_t i = 0;
            for(auto it = ans.begin();it!=ans.end();it++)
            {
                it->second.emplace_back(row[i++]);
            }
        }
        mysql_free_result(res);
    }
    {
        std::lock_guard<std::mutex> lock(m_pool_mutex);
        m_sqppool.push(conn);//用完的MYSQL对象返回到连接池中
    }
}

MYSQL *DataBase::createSQL()
{
    MYSQL* conn = mysql_init(conn);
    mysql_real_connect(conn,m_host,m_user,m_pwd,m_databasename,m_port,m_socket,m_clientFlag);
    m_conn_cnt++;
    return conn;
}

MYSQL *DataBase::getSQL()
{
    std::lock_guard<std::mutex> lock(m_pool_mutex);
    while(!m_sqppool.empty()&&mysql_ping(m_sqppool.front())){
        m_sqppool.pop();
        m_conn_cnt--;
    }
    if(m_sqppool.empty()){
        return createSQL();
    }
    else{
        MYSQL* conn = m_sqppool.front();
        m_sqppool.pop();
        return conn;
    }
}
