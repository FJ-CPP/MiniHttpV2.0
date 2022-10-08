#include <iostream>
#include <cassert>
#include "mysql.h"
#include "Common.hpp"
#include "Log.hpp"

using namespace std;

bool InsertSql(string &sql)
{
    MYSQL *conn = mysql_init(nullptr);
    mysql_set_character_set(conn, "utf8");
    if (mysql_real_connect(conn, "127.0.0.1", "http_user", "http12345", "CalculatorUser", 3306, nullptr, 0) == nullptr)
    {
        LOG(ERROR, "connect error!");
        return false;
    }
    LOG(INFO, "connect to mysql success!");
    int ret = mysql_query(conn, sql.c_str());
    mysql_close(conn);
    return ret == 0;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z')
    {
        y = x - 'A' + 10;
    }
    else if (x >= 'a' && x <= 'z')
    {
        y = x - 'a' + 10;
    }
    else if (x >= '0' && x <= '9')
    {
        y = x - '0';
    }
    else
    {
        assert(0);
    }
    return y;
}

std::string UrlDecode(const std::string &str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+')
        {
            strTemp += ' ';
        }
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else
        {
            strTemp += str[i];
        }
    }
    return strTemp;
}

int main()
{
    string method = getenv("METHOD");

    string query;
    GetQuery(method, query);
    // cerr << "query : " << query << endl;
    // query的格式为：name=?&password=?
    string q1;
    string q2;
    CutString(query, "&", q1, q2);
    // cerr << "q1"
    //      << " : " << q1 << endl;
    // cerr << "q2"
    //      << " : " << q2 << endl;

    string name, _name;
    string password, _password;
    CutString(q1, "=", name, _name);
    CutString(q2, "=", password, _password);
    _name = UrlDecode(_name);

    string sql = "insert into User (name, password) values (\'";
    sql += _name + "\', md5(\'";
    sql += _password + "\')";
    sql += ")";

    string res;
    if (InsertSql(sql))
    {
        res = "<p1>用户 " + _name + " 注册成功！</p1>";
    }
    else
    {
        res = "<p1>用户 " + _name + " 已存在！</p1>";
    }
    // 注册页面
    cout << "<!DOCTYPE html>";
    cout << "<html>";
    cout << "<meta charset=\"utf-8\">";
    cout << "<title>用户注册</title>";
    cout << "<head>";
    cout << res;    
    cout << "</head>";
    cout << "</html>";
    return 0;
}
