#pragma once
#include <signal.h>
#include "TcpSocket.h"
#include "Callback.h"

class HttpServer
{
private:
    uint16_t _port; // http服务端口号
public:
    HttpServer(uint16_t port);

    void Loop();
};