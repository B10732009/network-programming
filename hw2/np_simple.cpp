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

void getDefaultEnvVar(std::vector<std::string> &envVars, char **envVarPtr)
{
    envVars.clear();
    for (; (*envVarPtr); envVarPtr++)
    {
        std::string var(*envVarPtr);
        for (std::size_t i = 0; i < var.size(); i++)
        {
            if (var[i] == '=')
            {
                envVars.push_back(std::string(var.begin(), var.begin() + i));
                envVars.push_back(std::string(var.begin() + i + 1, var.end()));
                break;
            }
        }
    }
}

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

int main(int argc, char *argv[], char *envp[])
{
    if (argc != 2)
    {
        std::cerr << "usage ./np_simple <port>." << std::endl;
        return 1;
    }

    NpSocket npsocket(std::stoi(std::string(argv[1])));
    while (true)
    {
        int sock, port;
        std::string addr;
        while (!npsocket.npAccept(sock, addr, port))
            ;

        pid_t pid = fork();
        while (pid < 0)
        {
            perror("[ERROR] fork :");
            pid = fork();
        }

        if (pid == 0) // child
        {
            if (dup2(sock, 0) < 0)
                perror("[ERROR] dup stdin :");
            if (dup2(sock, 1) < 0)
                perror("[ERROR] dup stdout :");
            if (dup2(sock, 2) < 0)
                perror("[ERROR] dup stderr :");

            NpShell npshell;
            while (true)
            {
                std::cout << "% " << std::flush; // use std::flush to force to print the string
                std::string cmd;
                if (!std::getline(std::cin, cmd) || cmd.size() <= 0)
                {
                    perror("[ERROR] getline :");
                    break;
                }
                // delete invisible characters
                for (std::size_t i = 0; i < cmd.size(); i++)
                {
                    if (cmd[i] < 32)
                    {
                        cmd.erase(cmd.begin() + i);
                        i--;
                    }
                }
                // run command
                if (!(npshell.npRun(cmd)))
                    exit(0);
            }
            exit(1);
        }
        else // parent
        {
            if (close(sock) < 0)
                perror("[ERROR] close :");
            if (waitpid(pid, (int *)0, 0) < 0)
                perror("[ERROR] waitpid :");
        }
    }
    return 0;
}
