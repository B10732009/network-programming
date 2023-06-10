#include "user.hpp"

int User::mMaxSock = 0;
std::vector<int> User::mSockList;
std::vector<bool> User::mIdList(30, true);

User::User(std::string _name, int _sock, std::string _addr, int _port) //
    : mName(_name), mSock(_sock), mAddr(_addr), mPort(_port), mNpshell(NpShell(_sock))
{
    for (std::size_t i = 0; i < mIdList.size(); i++)
    {
        if (mIdList[i])
        {
            mId = i;
            mIdList[i] = false;
            return;
        }
    }

    mId = -1;
    std::cerr << "[ERROR] exceed user number limit." << std::endl;
}

bool User::run(std::string str)
{
    return mNpshell.npRun(str);
}

int User::id()
{
    return mId;
}

std::string User::name()
{
    return mName;
}

int User::sock()
{
    return mSock;
}

std::string User::addr()
{
    return mAddr;
}

int User::port()
{
    return mPort;
}