#pragma once
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "Log.hpp"
#include "Util.h"

class TcpServer
{
private:
    static TcpServer *_svr;
    uint16_t _port;
    int _listenSock;

private:
    TcpServer(uint16_t port);
    TcpServer(const TcpServer &o) = delete;
    TcpServer operator=(const TcpServer &) = delete;

    void Socket();

    void Bind();

    void Listen();

    void InitServer();

public:
    static TcpServer *GetInstance(uint16_t port);

    int GetListenSocket()
    {
        return _listenSock;
    }
    ~TcpServer()
    {
        if (_listenSock >= 0)
        {
            close(_listenSock);
        }
    }
};

