#include "npsocket.hpp"

NpSocket::NpSocket(int _port) : mSport(_port), mMaxsock(2)
{
    // create socket
    mSsock = socket(AF_INET, SOCK_STREAM, 0);
    if (mSsock < 0)
    {
        perror("[ERROR] socket :");
        return;
    }
    // set option to reuse address
    int optval = 1;
    if (setsockopt(mSsock, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) < 0)
    {
        perror("[ERROR] setsockopt :");
        return;
    }
    // bind
    sockaddr_in ssaddr;
    ssaddr.sin_family = AF_INET;
    ssaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ssaddr.sin_port = htons(mSport);
    std::memset(ssaddr.sin_zero, 0, sizeof(ssaddr.sin_zero));
    if (bind(mSsock, (const sockaddr *)(&ssaddr), sizeof(ssaddr)) < 0)
    {
        perror("[ERROR] bind :");
        return;
    }
    // listen (backlog(max number of client at same time) = 32)
    if (listen(mSsock, 32) < 0)
    {
        perror("[ERROR] listen :");
        return;
    }
    // add server socket to fdset
    npFdSet(mSsock);
    // update max number of socket
    mMaxsock = std::max(mSsock, mMaxsock);
}

bool NpSocket::npAccept(int &sock, std::string &addr, int &port)
{
    sockaddr_in csaddr;
    socklen_t csaddrLen = sizeof(csaddr);
    sock = accept(mSsock, (sockaddr *)(&csaddr), &csaddrLen);
    if (sock < 0)
    {
        perror("[ERROR] npAccept :");
        return false;
    }
    addr = inet_ntoa(csaddr.sin_addr);
    port = ntohs(csaddr.sin_port);
    npFdSet(sock);
    mMaxsock = std::max(mMaxsock, sock);
    return true;
}

int NpSocket::npSelect(fd_set &tempSet)
{
    std::memcpy(&tempSet, &mFdSet, sizeof(fd_set));
    int ret = select(mMaxsock + 1, &tempSet, (fd_set *)0, (fd_set *)0, (timeval *)0);
    if (ret < 0)
        perror("[ERROR] npSelect :");
    return ret;
}

void NpSocket::npFdSet(int fd)
{
    FD_SET(fd, &mFdSet);
}

void NpSocket::npFdClr(int fd)
{
    FD_CLR(fd, &mFdSet);
}

int NpSocket::npFdIsSet(int fd)
{
    return FD_ISSET(fd, &mFdSet);
}

int NpSocket::ssock()
{
    return mSsock;
}

int NpSocket::sport()
{
    return mSport;
}

int NpSocket::maxsock()
{
    return mMaxsock;
}