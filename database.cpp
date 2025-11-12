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


std::unordered_map<std::string, std::vector<const char *>> DataBase::executeSQL(const char *sql)
{
    MYSQL* conn = getSQL();
    std::unordered_map<std::string, std::vector<const char *>> ans;
    std::vector<std::string> fields;
    mysql_query(conn,sql);
    MYSQL_RES* res = mysql_store_result(conn);
    if(res!=nullptr)//是查询语句
    {
        MYSQL_FIELD* field;
        while(field = mysql_fetch_field(res))
        {
            std::string fieldname = field->name;
            fields.push_back(fieldname);
            ans[fieldname] = std::vector<const char *>();
        }
        MYSQL_ROW row;
        while(row = mysql_fetch_row(res))
        {
            for (size_t i = 0; i < fields.size(); ++i) {  // 按fields顺序遍历
                std::string fieldName = fields[i];  // 第i列的字段名
                const char* value = row[i] ? row[i] : "";  // 处理NULL
                ans[fieldName].push_back(value);  // 正确映射：row[i]→该字段的向量
            }
        }
        mysql_free_result(res);
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
