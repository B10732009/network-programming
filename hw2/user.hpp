#pragma once

#include <string>
#include <vector>

#include "npshell.hpp"

class User
{
  private:
    int mId;                          // user id
    int mSock;                        // user socket
    int mPort;                        // user port
    std::string mAddr;                // user address
    std::string mName;                // user name
    NpShell mNpshell;                 // independent npshell
    static std::vector<bool> mIdList; // record if the id is being used

  public:
    User() = delete;
    User(std::string _name, int _sock, std::string _addr, int _port);
    bool run(std::string str);

    int id();
    int sock();
    int port();
    std::string addr();
    std::string name();
    void setName(std::string name);
};