#pragma once

#include <string>
#include <vector>

#include "npshell.hpp"

class User
{
  private:
    int mId;
    std::string mName;
    int mSock;
    std::string mAddr;
    int mPort;

    NpShell mNpshell;

    static int mMaxSock;
    static std::vector<int> mSockList;
    static std::vector<bool> mIdList;

  public:
    User() = delete;
    User(std::string _name, int _sock, std::string _addr, int _port);
    bool run(std::string str);

    int id();
    std::string name();
    int sock();
    std::string addr();
    int port();
};