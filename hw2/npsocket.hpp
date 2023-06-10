#pragma once

#include <algorithm>
#include <cstdio>
#include <string>

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class NpSocket
{
  private:
    int mSsock;
    int mCsock;
    int mSport;
    int mMaxsock;
    fd_set mFdSet;

  public:
    NpSocket() = delete;
    NpSocket(int _port);
    bool npAccept(int &sock, std::string &addr, int &port);
    int npSelect(fd_set &tempSet);
    void npFdSet(int fd);
    void npFdClr(int fd);
    int npFdIsSet(int fd);

    int ssock();
    int csock();
    int sport();
    int maxsock();
};