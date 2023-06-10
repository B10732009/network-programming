#include <iostream>
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

                    if (userList[i].run(cmd)) // contiune
                        getWrite(userList[i].sock(), "% ");
                    else // logout
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
                }
            }
        }
    }

    return 0;
}