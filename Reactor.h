#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/epoll.h>
#include "Log.hpp"

class Reactor;
struct Event;

typedef void (*callback_t)(Event &); // 回调函数类型

struct Event
{
    Reactor *_r;          // reactor回指指针
    int _fd;              // 文件描述符
    std::string _rbuffer; // 读缓冲
    std::string _wbuffer; // 写缓冲

    callback_t _recv = nullptr;
    callback_t _send = nullptr;
    callback_t _error = nullptr;
    Event() = default;
    void RegisterCallback(callback_t recv, callback_t send, callback_t error); // 为Event注册回调函数
};

class Reactor
{
private:
    std::unordered_map<int, Event> _events; // 存储fd:Event的映射
    int _epfd;                              // epoll描述符
public:
    Reactor();
    void AddEvent(Event &ev, uint32_t events);
    void DelEvent(int fd);
    void ModEvent(int fd, bool readable, bool writable);
    void DispatchEvent(int timeout);
    ~Reactor();
};
