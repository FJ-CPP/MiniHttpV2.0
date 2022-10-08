#include <iostream>
#include "HttpServer.h"

void Usage(char** argv)
{
    std::cout << "Usage:" << "\n\t" << argv[0] << " port" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        Usage(argv);
        return -1;
    }

    uint16_t port = atoi(argv[1]);
    HttpServer* svr = new HttpServer(port);

    svr->Loop();
    return 0;
}