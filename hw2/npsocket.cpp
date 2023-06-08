#include "npsocket.hpp"

NpSocket::NpSocket(int _port) : ssock(-1), csock(-1), sport(_port)
{
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock < 0)
    {
        perror("[ERROR] socket :");
        return;
    }

    int optval = 1;
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) < 0)
    {
        perror("[ERROR] setsockopt :");
        return;
    }

    sockaddr_in ssaddr;
    ssaddr.sin_family = AF_INET;
    ssaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ssaddr.sin_port = htons(sport);
    std::memset(ssaddr.sin_zero, 0, sizeof(ssaddr.sin_zero));

    if (bind(ssock, (const sockaddr *)(&ssaddr), sizeof(ssaddr)) < 0)
    {
        perror("[ERROR] bind :");
        return;
    }

    if (listen(ssock, 32) < 0)
    {
        perror("[ERROR] listen :");
        return;
    }
}

int NpSocket::npAccept()
{
    sockaddr_in csaddr;
    socklen_t csaddrLen = sizeof(csaddr);
    csock = accept(ssock, (sockaddr *)(&csaddr), &csaddrLen);
    if (csock < 0)
    {
        perror("[ERROR] accept :");
        return false;
    }
    return csock;
}

// int NpSocket::getClientSock()
// {
//     return csock;
// }