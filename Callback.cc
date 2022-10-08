#include "Callback.h"

int ReceiverHelper(int fd, std::string &rbuffer)
{
    // 由于是ET模式，因此需要一次性读完所有数据
    while (1)
    {
        char buffer[1460];
        ssize_t s = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (s > 0)
        {
            // 正常读取到数据
            buffer[s] = 0;
            rbuffer += buffer;
        }
        else if (s == 0)
        {
            // 对端关闭连接，交给errorer回调函数处理
            return -1;
        }
        else
        {
            // recv出错
            if (errno == EINTR)
            {
                // 情况一、被信号中断
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 情况二、非阻塞返回，说明读取完毕
                break;
            }
            else
            {
                // 情况三、recv函数执行出错，交给错误处理回调errorer处理
                return -1;
            }
        }
    }
    return 0; // 成功返回
}

// 普通套接字的读回调
void Receiver(Event &event)
{
    // 将所有数据读入缓冲区
    if (ReceiverHelper(event._fd, event._rbuffer) == 0)
    {
        // 粘包处理
        EndPoint ep(event);
        while (ep.RecvHttpRequest())
        {
            // 进入循环说明本次接收到了完整的报文
            // 此时构建响应报文
            ep.BuildHttpResponse();
        }

        // 设置关注写事件，下一次将结果响应给client
        event._r->ModEvent(event._fd, true, true);
    }
    else
    {
        // 出错，直接交给error回调处理
        LOG(INFO, "Close Socket: " + std::to_string(event._fd));
        if (event._error)
        {
            event._error(event);
        }
    }
}

int SenderHelper(int fd, std::string &wbuffer)
{
    // std::cout << wbuffer << std::endl;
    size_t sz = wbuffer.size(); // 总的数据量
    size_t totalSize = 0;       // 已经发送的大小
    while (1)
    {
        // 所有数据已经全部发送
        if (totalSize == sz)
        {
            wbuffer = wbuffer.substr(totalSize);
            return 0;
        }
        char c = wbuffer[totalSize];
        ssize_t s = send(fd, &c, 1, 0);
        if (s > 0)
        {
            totalSize += s;
        }
        else
        {
            // 出错
            if (errno == EINTR)
            {
                // 情况一、被信号中断
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 情况二、说明对端缓冲区已被写满，此时停止
                wbuffer = wbuffer.substr(totalSize);
                return 1;
            }
            else
            {
                // 情况三、send函数执行出错
                return -1;
            }
        }
    }
}

// 普通套接字的写回调
void Sender(Event &event)
{
    int retCode = SenderHelper(event._fd, event._wbuffer);
    if (retCode == 0)
    {
        // 成功将所有数据写入对端
        event._r->ModEvent(event._fd, true, false); // 取消关注写事件
    }
    else if (retCode == 1)
    {
        LOG(INFO, "对端缓冲区已满");
        // 对端缓冲区满，有数据还没写完
        event._r->ModEvent(event._fd, true, true); // 继续关注写事件
    }
    else
    {
        LOG(ERROR, "写入出错，Close Socket: " + std::to_string(event._fd));
        // 出错，直接交给error回调处理
        if (event._error)
        {
            event._error(event);
        }
    }
}

// 普通套接字的异常回调
void Errorer(Event &event)
{
    event._r->DelEvent(event._fd);
}

// 监听套接字的读回调
void Accepter(Event &event)
{
    int listenSock = event._fd;

    // 由于是ET模式，所以有连接必须一次性处理
    while (1)
    {
        sockaddr_in peer;
        bzero(&peer, sizeof(peer));
        socklen_t len = sizeof(peer);
        int sock = accept(listenSock, (sockaddr *)&peer, &len);

        if (sock > 0)
        {
            // 新连接到来
            LOG(INFO, "新连接已建立：" + std::to_string(sock));
            // 添加到Reactor
            Event newEv;
            newEv._fd = sock;
            newEv._r = event._r;
            Util::NonBlock(sock); // 设置非阻塞
            newEv.RegisterCallback(Receiver, Sender, Errorer);
            event._r->AddEvent(newEv, EPOLLIN | EPOLLET); // 初始仅关心读事件，且设置成ET模式
        }
        else
        {
            // 出错
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 情况一、accept非阻塞返回，说明已经读取完毕，退出循环
                break;
            }
            else if (errno == EINTR)
            {
                // 情况二、accept被信号中断，继续读取即可
                continue;
            }
            else
            {
                // 情况三、真正的accept读取时出现问题
                LOG(ERROR, "Accept Error!");
            }
        }
    }
}