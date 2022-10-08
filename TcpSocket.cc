#include "TcpSocket.h"

static const int BACKLOG = 5;

TcpServer::TcpServer(uint16_t port)
    : _port(port), _listenSock(-1)
{
}

void TcpServer::Socket()
{
    _listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSock == -1)
    {
        LOG(FATAL, "Create listen socket error!");
        exit(1);
    }
    Util::NonBlock(_listenSock);
    Util::NonTimeWait(_listenSock);
}

void TcpServer::Bind()
{
    sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(_port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(_listenSock, (sockaddr *)&local, sizeof(local)) == -1)
    {
        LOG(FATAL, "Bind listen socket error!");
        exit(2);
    }
    LOG(INFO, "Bind listen socket success: " + std::to_string(_listenSock));
}

void TcpServer::Listen()
{
    if (listen(_listenSock, BACKLOG) == -1)
    {
        LOG(FATAL, "Listen error!");
        exit(3);
    }
    LOG(INFO, "Listen success");
}

void TcpServer::InitServer()
{
    Socket();
    Bind();
    Listen();
    LOG(INFO, "Init TcpServer success");
}

TcpServer *TcpServer::GetInstance(uint16_t port)
{
    static std::mutex mtx;
    if (_svr == nullptr)
    {
        mtx.lock();
        if (_svr == nullptr)
        {
            _svr = new TcpServer(port);
            _svr->InitServer();
        }
        mtx.unlock();
    }
    return _svr;
}

TcpServer *TcpServer::_svr = nullptr;
