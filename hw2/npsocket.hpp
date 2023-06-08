#pragma once

#include <cstdio>
#include <cstring>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class NpSocket
{
  private:
    int ssock;
    int csock;
    int sport;

  public:
    NpSocket() = delete;
    NpSocket(int _port);
    int npAccept();
    // int getClientSock();
};