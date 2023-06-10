#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "npshell.hpp"
#include "npsocket.hpp"
#include "user.hpp"

std::vector<User> userList;
std::vector<User> *User::mUserListPtr = &userList;

std::string getRead(int sock)
{
    char buf[15000];
    std::memset(buf, 0, 15000);
    if (read(sock, buf, 15000) < 0)
    {
        perror("[ERROR] getRead :");
        return "";
    }

    std::string ret = "";
    for (std::size_t i = 0; buf[i] != 0; i++)
    {
        if (buf[i] >= 32)
            ret += buf[i];
    }
    return ret;
}

void getWrite(int sock, std::string str)
{
    if (write(sock, str.c_str(), str.size()) < 0)
        perror("[ERROR] getWrite :");
}

void printLoginMsg(int id)
{
    for (auto u : userList)
    {
        if (u.id() == id)
        {
            getWrite(u.sock(), "****************************************\n");
            getWrite(u.sock(), "** Welcome to the information server. **\n");
            getWrite(u.sock(), "****************************************\n");
        }
        getWrite(u.sock(), "*** User '" + u.name() + "' entered from " + u.addr() + ":" + std::to_string(u.port()) + ". ***\n");
    }
}

void printLogoutMsg(int id)
{
    for (auto u : userList)
    {
        if (u.id() != id)
        {
            getWrite(u.sock(), "*** User '" + u.name() + "' left. ***\n");
        }
    }
}

void who(int id)
{
    for (auto u : userList)
    {
        if (u.id() == id)
        {
            std::sort(userList.begin(), userList.end(), [](User u1, User u2) { return u1.id() < u2.id(); });
            getWrite(u.sock(), "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
            for (auto v : userList)
                getWrite(u.sock(), std::to_string(v.id()) + "\t" + v.name() + "\t" + v.addr() + ":" + std::to_string(v.port()) + ((v.id() == id) ? "\t<-me" : "") + "\n");
        }
    }
}

void tell(int fid, int tid, std::string msg)
{
    auto fit = userList.end();
    auto tit = userList.end();
    for (auto it = userList.begin(); it != userList.end(); it++)
    {
        if (it->id() == fid)
            fit = it;
        if (it->id() == tid)
            tit = it;
    }
    if (tit != userList.end()) // receiver found
        getWrite(tit->sock(), "*** " + fit->name() + " told you ***: " + msg + "\n");
    else
        getWrite(fit->sock(), "*** Error: user #" + std::to_string(tid) + " does not exist yet. ***\n");
}

void yell(int id, std::string msg)
{
    for (auto u : userList)
    {
        if (u.id() == id)
        {
            for (auto v : userList)
                getWrite(v.sock(), "*** " + u.name() + " yelled ***: " + msg + "\n");
        }
    }
}

void name(int id, std::string name)
{
    // check if the new name already exists
    for (auto u : userList)
    {
        if (u.id() == id)
        {
            for (auto v : userList)
            {
                if (v.name() == name)
                    getWrite(u.sock(), "*** User '" + name + "' already exists. ***\n");
            }
        }
    }
    // update new name
    for (auto &u : userList)
    {
        if (u.id() == id)
            u.setName(name);
    }
}

int main(int argc, char *argv[], char *envp[])
{
    if (argc != 2)
    {
        std::cerr << "usage ./np_single_proc <port>." << std::endl;
        return 1;
    }

    NpSocket npsocket(std::stoi(std::string(argv[1])));
    while (true)
    {
        fd_set tempSet;
        while (npsocket.npSelect(tempSet) < 0)
            ;

        if (FD_ISSET(npsocket.ssock(), &tempSet)) // is from server socket
        {
            int sock, port;
            std::string addr;
            while (!npsocket.npAccept(sock, addr, port))
                ;

            User u("(no name)", sock, addr, port);
            if (u.id() != -1)
            {
                userList.push_back(u);
                printLoginMsg(u.id());
                getWrite(u.sock(), "% ");
            }
        }
        else // is from client socket
        {
            for (std::size_t i = 0; i < userList.size(); i++)
            {
                if (FD_ISSET(userList[i].sock(), &tempSet))
                {
                    std::string cmd = getRead(userList[i].sock());

                    if (dup2(userList[i].sock(), 1) < 0)
                        perror("[ERROR] dup stdout :");
                    if (dup2(userList[i].sock(), 2) < 0)
                        perror("[ERROR] dup stderr :");

                    if (cmd.substr(0, 3) == "who")
                        who(userList[i].id());
                    else if (cmd.substr(0, 4) == "tell")
                    {
                        std::stringstream ss;
                        std::string t, tid;
                        ss << cmd;
                        ss >> t >> tid;
                        // std::cerr << t << " " << tid << std::endl;
                        tell(userList[i].id(), std::stoi(tid), cmd.substr(6 + tid.size()));
                    }
                    else if (cmd.substr(0, 4) == "yell")
                        yell(userList[i].id(), cmd.substr(5));
                    else if (cmd.substr(0, 4) == "name")
                        name(userList[i].id(), cmd.substr(5));
                    else if (!userList[i].run(cmd)) // logout
                    {
                        printLogoutMsg(userList[i].id());
                        if (dup2(0, 1) < 0)
                            perror("[ERROR] dup stdout :");
                        if (dup2(0, 2) < 0)
                            perror("[ERROR] dup2 stderr :");
                        if (close(userList[i].sock()) < 0)
                            perror("[ERROR] close user socket :");
                        npsocket.npFdClr(userList[i].sock());
                        userList.erase(userList.begin() + i);
                        break;
                    }
                    getWrite(userList[i].sock(), "% ");
                    break;
                }
            }
        }
    }

    return 0;
}