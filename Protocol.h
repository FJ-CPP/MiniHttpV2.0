#pragma once
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include "Log.hpp"
#include "Util.h"
#include "Reactor.h"

const std::string WEB_ROOT = "wwwroot";
const std::string HOME_PAGE = "index.html";
const std::string NOT_FOUND_PAGE = "404.html";

const std::string HTTP_VERSION = "HTTP/1.1";
const std::string END_LINE = "\r\n";

enum StatusCode
{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500
};

static std::string Code2Desc(int code);

static std::string Suffix2Type(std::string suffix);

struct HttpRequest
{
    // 解析前的报文内容
    std::string _requestLine;                // 请求行
    std::vector<std::string> _requestHeader; // 请求报头
    std::string _blank;                      // 空行
    std::string _requestBody;                // 请求正文

    // 解析后的报文内容
    std::string _method;                                  // 请求方法
    std::string _uri;                                     // 请求uri
    std::string _version;                                 // http版本
    std::unordered_map<std::string, std::string> _header; // 请求报头
    int _contentLength;                                   // 正文长度

    // uri的解析结果
    std::string _path;  // 资源路径
    std::string _query; // 请求参数

    bool _cgi; // 该请求是否需要cgi处理
    HttpRequest();
};

struct HttpResponse
{
    std::string _statusLine;                  // 状态行
    std::vector<std::string> _responseHeader; // 响应报头
    std::string _blank;                       // 空行
    std::string _responseBody;                // 响应正文
    int _statusCode;                          // 状态码
    size_t _fd;                               // 请求访问的目标文件fd
    std::string _suffix;                      // 请求访问的目标文件后缀
    size_t _contentLength;                    // 响应正文的长度

    HttpResponse();
    ~HttpResponse();
};

class EndPoint
{
private:
    Event &_event;              // 读写缓冲区
    HttpRequest _httpRequest;   // 请求报文
    HttpResponse _httpResponse; // 响应报文

private:
    // 接收请求行
    int RecvHttpRequestLine(int start);
    // 接收请求报头
    int RecvHttpRequestHeader(int start);
    // 判断是否需要接收请求正文
    bool NeedToRecvHttpRequestBody();
    // 接收请求正文
    int RecvHttpRequestBody(int start);
    // 解析请求行
    void ParseHttpRequestLine();
    // 解析请求报头
    void ParseHttpRequestHeader();
    // 处理非CGI请求
    int ProcessNonCgi();
    // 处理CGI请求
    int ProcessCgi();

    void BuildHttpResponseHelper();

    void BuildOkResponse();

    void BuildErrorResponse(std::string page);

public:
    EndPoint(Event &event);
    // 接收请求报文
    bool RecvHttpRequest();
    // 构建响应报文
    void BuildHttpResponse();
};
