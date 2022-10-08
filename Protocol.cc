#include "Protocol.h"

static std::string Code2Desc(int code)
{
    std::unordered_map<int, std::string> desc = {
        {OK, "OK"},
        {BAD_REQUEST, "Bad Request"},
        {NOT_FOUND, "Not Found"},
        {INTERNAL_SERVER_ERROR, "Internal Server Error"}};
    if (desc.find(code) != desc.end())
    {
        return desc[code];
    }
    return "OK";
}

static std::string Suffix2Type(std::string suffix)
{
    std::unordered_map<std::string, std::string> type = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".xml", "application/xml"},
        {".jpg", "application/x-jpg"},
        {".ico", "application/x-ico"}, 
        {".gif", "image/gif"}};
    if (type.find(suffix) != type.end())
    {
        return type[suffix];
    }
    return "text/html";
}

HttpRequest::HttpRequest()
    : _cgi(false)
{
}

HttpResponse::HttpResponse()
    : _blank(END_LINE), _statusCode(OK), _fd(-1)
{
}

HttpResponse::~HttpResponse()
{
    if (_fd >= 0)
    {
        close(_fd);
    }
}

EndPoint::EndPoint(Event &event)
    : _event(event)
{
}

int EndPoint::RecvHttpRequestLine(int start)
{
    auto &line = _httpRequest._requestLine;
    std::string buffer = _event._rbuffer;
    int res = Util::GetLine(buffer, start, line);
    // std::cout << line << std::endl;
    return res;
}

int EndPoint::RecvHttpRequestHeader(int start)
{
    std::string line;
    std::string buffer = _event._rbuffer;
    // 按行读取请求报头，遇到'\n'停止
    while (1)
    {
        int res = Util::GetLine(buffer, start, line);
        if (res == -1)
        {
            return -1;
        }
        else
        {
            start = res;
            // std::cout << line << std::endl;
            if (line == "") // 读到分隔正文和报头的空行
            {
                break;
            }
            _httpRequest._requestHeader.push_back(line);
        }
    }

    if (line == "")
    {
        _httpRequest._blank = "\n";
        return start;
    }
    return -1;
}

bool EndPoint::NeedToRecvHttpRequestBody()
{
    if (_httpRequest._method == "POST")
    {
        if (_httpRequest._header.find("Content-Length") != _httpRequest._header.end())
        {
            _httpRequest._contentLength = stoi(_httpRequest._header["Content-Length"]);
            return true;
        }
    }
    return false;
}
int EndPoint::RecvHttpRequestBody(int start)
{
    if (NeedToRecvHttpRequestBody())
    {
        int n = _httpRequest._contentLength;
        if (start + n > _event._rbuffer.size())
        {
            LOG(INFO, "正文不全");
            return -1;
        }

        _httpRequest._requestBody = _event._rbuffer.substr(start, n);
        // std::cout << _httpRequest._requestBody << std::endl;
        start += n;
    }
    return start;
}

void EndPoint::ParseHttpRequestLine()
{
    auto &line = _httpRequest._requestLine;
    std::stringstream ss(line);
    ss >> _httpRequest._method >> _httpRequest._uri >> _httpRequest._version;
    // 将请求方法统一转为大写
    auto &method = _httpRequest._method;
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
}
void EndPoint::ParseHttpRequestHeader()
{
    std::string key;
    std::string value;

    for (auto &line : _httpRequest._requestHeader)
    {
        // 请求报头格式为key: value
        Util::CutString(line, ": ", key, value);
        _httpRequest._header[key] = value;
    }
}

int EndPoint::ProcessNonCgi()
{
    // 对于非cgi的请求，只需打开目标文件即可
    _httpResponse._fd = open(_httpRequest._path.c_str(), O_RDONLY);
    if (_httpResponse._fd >= 0)
    {
        return OK;
    }
    return NOT_FOUND;
}

int EndPoint::ProcessCgi()
{
    int retCode = OK;
    // 将请求方法写入环境变量，方便子进程获取
    std::string methodEnv = "METHOD=" + _httpRequest._method;
    std::string queryEnv = "QUERY=" + _httpRequest._query;
    putenv((char *)methodEnv.c_str());
    // GET方法的请求参数在uri中，而uri限定为2048字节，因此用环境变量传递更为高效
    if (_httpRequest._method == "GET")
    {
        queryEnv = "QUERY=" + _httpRequest._query;
        putenv((char *)queryEnv.c_str()); // putenv需要注意字符串的生命周期！！！
    }
    else if (_httpRequest._method == "POST")
    {
        // 如果是POST方法，则将正文长度以环境变量的方式传给子进程
        queryEnv = "CONTENT_LENGTH=" + std::to_string(_httpRequest._contentLength);
        putenv((char *)queryEnv.c_str());
    }
    int input[2];  // 父进程读取管道
    int output[2]; // 父进程写入管道

    if (pipe(input) == -1 || pipe(output) == -1)
    {
        LOG(ERROR, "Pipe error!");
        return INTERNAL_SERVER_ERROR;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // 子进程往output[0]写，从intput[1]读，其余管道读写端关闭
        close(input[0]);
        close(output[1]);
        // 为了避免程序替换后子进程不知道管道读写端描述符，因此约定：
        // fd=0用来读父进程传来的数据，fd=1用来向父进程写数据
        dup2(output[0], 0);
        dup2(input[1], 1);

        execl(_httpRequest._path.c_str(), _httpRequest._path.c_str(), nullptr);
        LOG(ERROR, "Execl error: " + std::string(strerror(errno)));
        exit(0);
    }
    else if (pid > 0)
    {
        // 父进程从input[0]读，往output[1]写，其余管道读写端关闭
        close(input[1]);
        close(output[0]);
        // POST方法的请求参数在报文体中，可能较长，因此需要用管道向子进程传递请求参数
        if (_httpRequest._method == "POST")
        {
            ssize_t s = 0;
            ssize_t total = 0;
            auto &body = _httpRequest._requestBody;
            const char *start = body.c_str();
            // 避免管道满导致一次写不完所有数据，因此循环写入数据
            while (total < body.size() && (s = write(output[1], start + total, body.size() - total)) > 0)
            {
                total += s;
            }
        }

        // 从子进程获取响应正文
        char ch = 0;
        while (read(input[0], &ch, 1) > 0)
        {
            _httpResponse._responseBody.push_back(ch);
        }
        _httpResponse._contentLength = _httpResponse._responseBody.size();

        int status = 0;
        waitpid(pid, &status, 0); // 阻塞等待子进程退出
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 0)
            {
                // 子进程成功执行
                retCode = OK;
            }
            else
            {
                retCode = BAD_REQUEST;
            }
        }
        else
        {
            LOG(ERROR, "WIFEXITED failed, sig:" + std::to_string(WTERMSIG(status)));
            retCode = NOT_FOUND;
        }
        // 线程结束不会关闭fd，所以必须手动关闭管道文件，避免套接字泄露
        close(input[0]);
        close(output[1]);
    }
    else
    {
        LOG(ERROR, "Fork error!");
        return INTERNAL_SERVER_ERROR;
    }
    return retCode;
}

void EndPoint::BuildHttpResponseHelper()
{
    // 构建状态行
    _httpResponse._statusLine += HTTP_VERSION; // HTTP版本
    _httpResponse._statusLine += " ";
    _httpResponse._statusLine += std::to_string(_httpResponse._statusCode); // 状态码
    _httpResponse._statusLine += " ";
    _httpResponse._statusLine += Code2Desc(_httpResponse._statusCode); // 状态码描述
    _httpResponse._statusLine += _httpResponse._blank;

    // 根据状态码构建响应报头和响应正文
    switch (_httpResponse._statusCode)
    {
    case OK:
        BuildOkResponse();
        break;
    case NOT_FOUND:
        BuildErrorResponse(NOT_FOUND_PAGE);
    default:
        break;
    }

    // 将响应报文加入缓冲区
    // 响应行
    _event._wbuffer += _httpResponse._statusLine;
    // 响应报头
    for (auto &line : _httpResponse._responseHeader)
    {
        _event._wbuffer += line;
    }
    // 空行
    _event._wbuffer += _httpResponse._blank;
    // 正文
    if (_httpRequest._cgi)
    {
        // cgi处理的结果存储在body中
        _event._wbuffer += _httpResponse._responseBody;
    }
    else
    {
        // 非cgi处理的结果就是普通目标文件的内容
        int fd = _httpResponse._fd;
        char c;
        ssize_t s = 0;
        while ((s = read(fd, &c, 1)) > 0)
        {
            _event._wbuffer += c;
        }
    }
}

void EndPoint::BuildOkResponse()
{
    // 响应报头
    std::string line;
    line = "Content-Length: " + std::to_string(_httpResponse._contentLength) + _httpResponse._blank;
    _httpResponse._responseHeader.emplace_back(line);
    line = "Content-Type: " + Suffix2Type(_httpResponse._suffix) + _httpResponse._blank;
    _httpResponse._responseHeader.emplace_back(line);
}

void EndPoint::BuildErrorResponse(std::string page)
{
    std::string line;
    page = WEB_ROOT + "/" + page; // 访问web根目录下的错误页面
    _httpResponse._fd = open(page.c_str(), O_RDONLY);
    if (_httpResponse._fd > 0)
    {
        struct stat st;
        stat(page.c_str(), &st);
        _httpResponse._contentLength = st.st_size;
        line = "Content-Type: text/html" + END_LINE;
        _httpResponse._responseHeader.emplace_back(line);
        line = "Content-Length: " + std::to_string(st.st_size) + END_LINE;
        _httpResponse._responseHeader.emplace_back(line);
    }
    _httpRequest._cgi = false; // 下一步发送普通文件，而非cgi的处理结果
}

bool EndPoint::RecvHttpRequest()
{
    int start = 0;
    int cur = RecvHttpRequestLine(start);
    cur = RecvHttpRequestHeader(cur);
    if (cur == -1)
    {
        return false;
    }
    ParseHttpRequestLine();
    ParseHttpRequestHeader();
    cur = RecvHttpRequestBody(cur);
    if (cur == -1)
    {
        return false;
    }
    _event._rbuffer = _event._rbuffer.substr(cur);
    return true;
}

void EndPoint::BuildHttpResponse()
{
    // 获取请求目标文件的路径
    if (_httpRequest._method == "GET")
    {
        // 解析请求uri：如果GET方法有请求参数，则请求路径和请求参数会以'?'作为分隔
        if (_httpRequest._uri.find('?') != std::string::npos)
        {
            Util::CutString(_httpRequest._uri, "?", _httpRequest._path, _httpRequest._query);
            _httpRequest._cgi = true; // 有请求参数，需要cgi介入
        }
        else
        {
            _httpRequest._path = _httpRequest._uri;
        }
    }
    else if (_httpRequest._method == "POST")
    {
        _httpRequest._path = _httpRequest._uri;
        _httpRequest._cgi = true; // 有请求参数，需要cgi介入
    }
    else
    {
        // 仅处理GET和POST方法
        LOG(WARNING, "Method not found!");
        _httpResponse._statusCode = BAD_REQUEST;
        goto END;
    }
    // 以WEB_ROOT为根目录搜索路径
    _httpRequest._path = WEB_ROOT + _httpRequest._path;
    // 路径以'/'结尾，说明该路径可能是目录，且无指定文件，默认返回目录主页
    if (_httpRequest._path[_httpRequest._path.size() - 1] == '/')
    {
        _httpRequest._path += HOME_PAGE;
    }
    // 根据路径查看目标文件的属性信息
    struct stat st;
    if (stat(_httpRequest._path.c_str(), &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            // 如果目标文件是一个目录，则默认访问该目录下的主页
            _httpRequest._path.append("/" + HOME_PAGE);
            // 获取主页的属性信息
            stat(_httpRequest._path.c_str(), &st);
        }
        else if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
        {
            // 如果目标文件是一个可执行程序，则以cgi方式处理响应
            _httpRequest._cgi = true;
        }
        _httpResponse._contentLength = st.st_size; // 返回普通文件时，响应正文长度就是文件大小
        // 获取目标文件的后缀
        int pos = _httpRequest._path.rfind(".");
        if (pos == std::string::npos)
        {
            _httpResponse._suffix = ".html";
        }
        else
        {
            _httpResponse._suffix = _httpRequest._path.substr(pos);
        }
    }
    else
    {
        // 目标文件不存在
        LOG(WARNING, _httpRequest._path + " Not Found!");
        _httpResponse._statusCode = NOT_FOUND;
        goto END;
    }
    // 根据目标文件是否需要cgi进行分类处理
    if (_httpRequest._cgi == true)
    {
        _httpResponse._statusCode = ProcessCgi();
    }
    else
    {
        _httpResponse._statusCode = ProcessNonCgi();
    }
END:
    // 构建响应报文
    BuildHttpResponseHelper();
}
