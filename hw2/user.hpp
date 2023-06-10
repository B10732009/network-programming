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

    void userWho();
    void userTell(int id);
    void userYell();
    void userName(std::string name);

  public:
    static std::vector<User> *mUserListPtr;

    User() = delete;
    User(std::string _name, int _sock, std::string _addr, int _port);
    bool run(std::string str);

    int id();
    std::string name();
    int sock();
    std::string addr();
    int port();

    void setName(std::string name);
};