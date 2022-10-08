#pragma once
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

// 功能函数类
class Util
{
public:
    // 返回下一次处理的起始位置
    static int GetLine(std::string &buffer, int start, std::string &line);

    static void CutString(const std::string &str, const std::string &sep, std::string &sub1, std::string &sub2);

    static void NonBlock(int fd);

    static void NonTimeWait(int fd);
};