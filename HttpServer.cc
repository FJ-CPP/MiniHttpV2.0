#include "HttpServer.h"
HttpServer::HttpServer(uint16_t port)
    : _port(port)
{
    // 忽略SIGPIPE信号，避免因为客户端崩溃时，服务端向客户端写数据，从而导致服务端收到SIGPIPE而被kill
    signal(SIGPIPE, SIG_IGN);
    LOG(INFO, "Init HttpServer Success!");
}

void HttpServer::Loop()
{
    // 将监听套接字及其关注事件加入反应堆
    Reactor *r = new Reactor();
    TcpServer* tcp = TcpServer::GetInstance(_port);
    Event event;
    event._fd = tcp->GetListenSocket();
    event._r = r;
    event.RegisterCallback(Accepter, nullptr, nullptr);
    r->AddEvent(event, EPOLLIN | EPOLLET);

    while (1)
    {
        r->DispatchEvent(1000);
    }
    
}