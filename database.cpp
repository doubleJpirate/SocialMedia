#include "database.h"
#include"iostream"

std::mutex DataBase::m_obj_mutex;
std::mutex DataBase::m_pool_mutex;
DataBase* DataBase::m_database = nullptr;

DataBase::~DataBase()
{
    std::lock_guard<std::mutex> lock(m_pool_mutex);
    while(!m_sql_pool.empty()){
        MYSQL* conn = m_sql_pool.front();
        m_sql_pool.pop();
        mysql_close(conn);
        m_conn_cnt--;
    }
}

DataBase *DataBase::getInstance()
{
    if(m_database==nullptr){
        std::lock_guard<std::mutex> lock(m_obj_mutex);
        if(m_database == nullptr){
            m_database = new DataBase();
        }
    }
    return m_database;
}

void DataBase::init(const char *host, const char *user, const char *pwd, const char *databasename, unsigned int port, const char *serversocket, unsigned long flag)
{
    m_user = user;
    m_pwd = pwd;
    m_databasename = databasename;
    m_port = port;
    m_socket = serversocket;
    m_clientFlag = flag;
    m_host = host;
}


std::map<std::string, std::vector<const char *>> DataBase::executeSQL(const char *sql)
{
    MYSQL* conn = getSQL();
    if(conn==nullptr)std::cout<<"创建失败aaa"<<std::endl;
    std::cout<<"创建成功aaa"<<std::endl;
    if(mysql_ping(conn))std::cout<<"无法连接"<<std::endl;
    std::cout<<"连接成功"<<std::endl;
    std::map<std::string, std::vector<const char *>> ans;
    int r = mysql_query(conn,sql);
    std::cout<<r<<std::endl;
    std::cerr << "Failed to select database: " << mysql_error(conn) << std::endl;
    MYSQL_RES* res = mysql_store_result(conn);
    if(res!=nullptr)//是查询语句
    {
        std::cout<<"有结果"<<std::endl;
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
    else{
        std::cout<<"无结果"<<std::endl;
    }
    {
        std::lock_guard<std::mutex> lock(m_pool_mutex);
        m_sql_pool.push(conn);//用完的MYSQL对象返回到连接池中
    }
    return ans;
}

MYSQL *DataBase::createSQL()
{
    MYSQL* conn = mysql_init(nullptr);
    if(conn==nullptr){
        std::cout<<"创建失败"<<std::endl;
    }
    std::cout<<"创建成功"<<std::endl;
    mysql_real_connect(conn,m_host,m_user,m_pwd,m_databasename,m_port,m_socket,m_clientFlag);
    m_conn_cnt++;
    return conn;
}

MYSQL *DataBase::getSQL()
{
    std::lock_guard<std::mutex> lock(m_pool_mutex);
    while(!m_sql_pool.empty()&&mysql_ping(m_sql_pool.front())){
        mysql_close(m_sql_pool.front());
        m_sql_pool.pop();
        m_conn_cnt--;
    }
    if(m_sql_pool.empty()){
        return createSQL();
    }
    else{
        MYSQL* conn = m_sql_pool.front();
        m_sql_pool.pop();
        return conn;
    }
}
