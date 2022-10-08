#include "Util.h"

int Util::GetLine(std::string &buffer, int start, std::string &line)
{
    if (start == -1)
    {
        return -1;
    }
    int n = buffer.size();
    int i = start;
    while (i < n)
    {
    // 将报文按照行进行读取，其中不同系统的行末字符不同，可能为'\n'、'\r'或'\r\n'
        if (buffer[i] == '\n')
        {
            line = buffer.substr(start, i - start); // 去掉\n
            return i + 1;
        }
        if (buffer[i] == '\r')
        {
            if (i + 1 < n)
            {
                if (buffer[i + 1] == '\n')
                {
                    // 以\r\n换行
                    line = buffer.substr(start, i - start); // 去掉\r\n
                    return i + 2;
                }
                else
                {
                    // 以\r换行
                    line = buffer.substr(start, i - start); // 去掉\r\n
                    return i + 1;
                }
            }
            else
            {
                // 可能是\r\n结尾，但是报文不全，因此返回-1
                return -1;
            }
        }
        ++i;
    }  
    return -1;
}

void Util::CutString(const std::string &str, const std::string &sep, std::string &sub1, std::string &sub2)
{
    int pos = str.find(sep);
    sub1 = str.substr(0, pos);
    sub2 = str.substr(pos + sep.size());
}

void Util::NonBlock(int fd)
{
    int flg = fcntl(fd, F_GETFL);
    flg |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flg);
}

void Util::NonTimeWait(int fd)
{
    // 设置地址复用(取消TIME_WAIT，方便立刻重新启用端口)
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}