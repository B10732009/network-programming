#include "user.hpp"

std::vector<bool> User::mIdList(30, true); // max number of user = 30

User::User(std::string _name, int _sock, std::string _addr, int _port) //
    : mSock(_sock), mPort(_port), mAddr(_addr), mName(_name), mNpshell(NpShell())
{
    // choose a id that isn't being used
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

int User::sock()
{
    return mSock;
}

int User::port()
{
    return mPort;
}

std::string User::addr()
{
    return mAddr;
}

std::string User::name()
{
    return mName;
}

void User::setName(std::string name)
{
    mName = name;
}