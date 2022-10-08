#include "Reactor.h"

void Event::RegisterCallback(callback_t recv, callback_t send, callback_t error)
{
    _recv = recv;
    _send = send;
    _error = error;
}

Reactor::Reactor()
{
    _epfd = epoll_create(256);
    if (_epfd == -1)
    {
        LOG(FATAL, "Create epoll error");
        exit(-1);
    }
}

void Reactor::AddEvent(Event &ev, uint32_t events)
{
    int sock = ev._fd;
    _events[sock] = ev;

    epoll_event epollEv;
    epollEv.data.fd = sock;
    epollEv.events = events;

    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, sock, &epollEv) == -1)
    {
        exit(4);
    }
}

void Reactor::DelEvent(int fd)
{
    if (_events.find(fd) != _events.end())
    {
        epoll_event ev;
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &ev) == -1)
        {
            exit(5);
        }
        _events.erase(fd);
        close(fd);
    }
}

void Reactor::ModEvent(int fd, bool readable, bool writable)
{
    // 根据readable和writable设置读写状态
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = (readable ? EPOLLIN : 0) | (writable ? EPOLLOUT : 0);  
    epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &ev);
}

void Reactor::DispatchEvent(int timeout)
{
    const int NUM = 128;
    epoll_event events[NUM];
    int n = epoll_wait(_epfd, events, NUM, timeout);

    for (int i = 0; i < n; ++i)
    {
        int sock = events[i].data.fd;
        uint32_t evts = events[i].events;
        if (_events.find(sock) != _events.end() && (evts & EPOLLERR) && _events[sock]._error)
        {
            _events[sock]._error(_events[sock]);
        }
        if (_events.find(sock) != _events.end() && (evts & EPOLLHUP) && _events[sock]._error)
        {
            _events[sock]._error(_events[sock]);
        }
        if (_events.find(sock) != _events.end() && (evts & EPOLLIN) && _events[sock]._recv)
        {
            _events[sock]._recv(_events[sock]);
        }
        if (_events.find(sock) != _events.end() && (evts & EPOLLOUT) && _events[sock]._send)
        {
            _events[sock]._send(_events[sock]);
        }
    }
}

Reactor::~Reactor()
{
    close(_epfd);
}