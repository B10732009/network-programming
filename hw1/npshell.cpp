#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
// #include <map>
#include <string>
#include <vector>

// #include <stdio.h>
// #include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// #include "types/cmd.hpp"
// #include "types/proc.hpp"

#define MAX_CMD_ARG 32
#define MAX_CMD_LEN 15000

#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERR_FD 2

#define PIPE_READ 0
#define PIPE_WRITE 1

class Proc
{
  public:
    pid_t pid;
    int readEnd;
    int writeEnd;
    Proc() = delete;
    Proc(pid_t _pid, int _readEnd, int _writeEnd) : pid(_pid), readEnd(_readEnd), writeEnd(_writeEnd)
    {
    }
};

class Cmd
{
  public:
    std::vector<std::string> data;
    int cnt;
    char type;
    Cmd() = delete;
    Cmd(std::vector<std::string> _data, int _cnt, char _type) : data(_data), cnt(_cnt), type(_type)
    {
    }
};

class Numpipe
{
  public:
    int read;
    int write;
    int cnt;
    Numpipe(int _read, int _write, int _cnt) : read(_read), write(_write), cnt(_cnt)
    {
    }
};

std::vector<Proc> procList;
std::vector<Numpipe> numpipeList;

void npParse(std::vector<Cmd> &cmds, std::string &str)
{
    cmds = std::vector<Cmd>({{{}, 0, '0'}});
    std::string delim = " ";
    std::string::size_type begin = 0;
    std::string::size_type end = str.find(delim);
    while (end != std::string::npos)
    {
        if (begin < end)
        {
            std::string temp = str.substr(begin, end - begin);
            if (*(temp.begin()) == '|' || *(temp.begin()) == '!')
            {
                if (temp.size() > 1)
                {
                    (*(cmds.end() - 1)).cnt = std::stol(std::string(temp.begin() + 1, temp.end()));
                    (*(cmds.end() - 1)).type = *(temp.begin());
                }
                // (*(cmds.end() - 1)).type = *(temp.begin());
                cmds.push_back({{}, 0, '0'});
            }
            else
            {
                (*(cmds.end() - 1)).data.push_back(temp);
            }
        }
        begin = end + delim.size();
        end = str.find(delim, begin);
    }
    if (begin < str.size())
    {
        std::string temp = str.substr(begin);
        if (*(temp.begin()) == '|' || *(temp.begin()) == '!')
        {
            if (temp.size() > 1)
            {
                (*(cmds.end() - 1)).cnt = stol(std::string(temp.begin() + 1, temp.end()));
                (*(cmds.end() - 1)).type = *(temp.begin());
            }
            // (*(cmds.end() - 1)).type = *(temp.begin());
        }
        else
        {
            (*(cmds.end() - 1)).data.push_back(temp);
        }
    }
}

void npSetenv(std::string var, std::string value)
{
    if (setenv(var.c_str(), value.c_str(), 1) < 0)
        perror("[ERROR] setenv : ");
}

void npPrintenv(std::string var)
{
    char *env = getenv(var.c_str());
    if (env)
        std::cout << env << std::endl;
}

void npExec(std::vector<std::string> cmds)
{
    char args[MAX_CMD_ARG][MAX_CMD_LEN];
    char *argp[MAX_CMD_ARG];
    for (std::size_t i = 0; i < cmds.size(); i++)
        std::strcpy(args[i], cmds[i].c_str());
    for (std::size_t i = 0; i < cmds.size(); i++)
        argp[i] = args[i];
    argp[cmds.size()] = (char *)0;

    if (execvp(argp[0], argp) < 0)
        std::cerr << "Unknown command: [" << argp[0] << "]." << std::endl;
    exit(1);
}

void npExit()
{
    exit(0);
}

void npDup(int fd1, int fd2)
{
    if (dup2(fd1, fd2) < 0)
        perror("[ERROR] npDup :");
    // else
    //     std::cout << "dup " << fd1 << " " << fd2 << std::endl;
}

void npClose(int fd)
{
    if (close(fd) < 0)
        perror("[ERROR] npClose :");
    // else
    //     std::cout << "close " << fd << std::endl;
}

bool npWait(std::vector<Proc> &procs)
{
    bool waitFlag = false;
    for (std::size_t i = 0; i < procs.size(); i++)
    {
        int opt = (procs[i].writeEnd == STDOUT_FD) ? 0 : WNOHANG;
        pid_t wpid = waitpid(procs[i].pid, (int *)0, opt);
        if (wpid > 0)
        {
            procs.erase(procs.begin() + i);
            waitFlag = true;
            i--;
        }
    }
    return waitFlag;
}

int main()
{
    // initialize PATH
    npSetenv("PATH", "bin:.");

    // main loop
    while (true)
    {
        std::cout << "% ";
        std::string str;
        if (!std::getline(std::cin, str))
            break;
        if (str.size() == 0)
            continue;

        // parse the input string
        std::vector<Cmd> cmds;
        npParse(cmds, str);

        // for (auto elem : cmds)
        // {
        //     std::cout << elem.cnt << " " << elem.type << " ";
        //     for (auto j : elem.data)
        //         std::cout << j << " ";
        //     std::cout << std::endl;
        // }

        for (auto &numpipe : numpipeList)
            numpipe.cnt--;

        // built-in functions
        if (cmds.size() == 1 && cmds[0].data.size() == 3 && cmds[0].data[0] == "setenv")
        {
            npSetenv(cmds[0].data[1], cmds[0].data[2]);
            continue;
        }
        else if (cmds.size() == 1 && cmds[0].data.size() == 2 && cmds[0].data[0] == "printenv")
        {
            npPrintenv(cmds[0].data[1]);
            continue;
        }
        else if (cmds.size() == 1 && cmds[0].data.size() == 1 && cmds[0].data[0] == "exit")
        {
            npWait(procList);
            npExit();
        }

        // read-end FD of last round, initially set to STDIN
        int prevRead = STDIN_FD;

        for (std::size_t i = 0; i < cmds.size(); i++)
        {
            bool isFirst = (i == 0);
            bool isLast = (i == cmds.size() - 1);
            bool isFile = (std::find(cmds[i].data.begin(), cmds[i].data.end(), ">") != cmds[i].data.end());
            bool isNumPipe1 = (cmds[i].type == '|');
            bool isNumPipe2 = (cmds[i].type == '!');
            bool isNumPipe = isNumPipe1 || isNumPipe2;
            bool isOrdPipe = !isNumPipe;
            bool isPipe;

            bool isReadFromNumPipe = false;
            bool isWriteToNumPipe = false;
            int readFromNumPipeWrite = -1;
            int writeToNumPipeWrite = -1;

            for (std::size_t j = 0; j < numpipeList.size(); j++)
            {
                if (numpipeList[j].cnt <= 0)
                {
                    isReadFromNumPipe = true;
                    readFromNumPipeWrite = numpipeList[j].write;
                    prevRead = numpipeList[j].read;
                    numpipeList.erase(numpipeList.begin() + j);
                    break;
                }
            }

            if (isNumPipe)
            {
                for (std::size_t j = 0; j < numpipeList.size(); j++)
                {
                    if (numpipeList[j].cnt == cmds[i].cnt)
                    {
                        isWriteToNumPipe = true;
                        writeToNumPipeWrite = numpipeList[j].write;
                        break;
                    }
                }
            }

            // create pipe
            int pipedes[2] = {-1, -1};
            isPipe = (isOrdPipe && !isLast) || (isNumPipe && !isWriteToNumPipe) || isFile;
            if (isPipe)
            {
                while (pipe(pipedes) < 0)
                    perror("[ERROR] pipe : ");
            }

            // fork
            while (true)
            {
                pid_t pid = fork();
                // error
                if (pid == -1)
                {
                    while (npWait(procList))
                        ;
                }
                // child process
                else if (pid == 0)
                {
                    // dup prevRead to STDIN
                    npDup(prevRead, STDIN_FD);

                    // close the prevRead,
                    // note that if prevRead is same as STDIN, don't close
                    if (prevRead != STDIN_FD)
                        npClose(prevRead);

                    if (isReadFromNumPipe)
                        npClose(readFromNumPipeWrite);

                    // close unused pipe read-end
                    // if (isPipe)
                    //     npClose(pipedes[PIPE_READ]);

                    if (isFile)
                    {
                        int ffd;
                        while ((ffd = open((*(cmds[i].data.end() - 1)).c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0)
                            perror("[ERROR] open :");
                        npDup(ffd, STDOUT_FD);
                        npClose(ffd);
                        npExec(std::vector<std::string>(cmds[i].data.begin(), cmds[i].data.end() - 2));
                        break; // never reach here
                    }

                    // pipe() called in this round
                    if (isPipe)
                    {
                        // std::cout << "isPipe" << std::endl;
                        npDup(pipedes[PIPE_WRITE], STDOUT_FD);
                        if (isNumPipe2)
                            npDup(pipedes[PIPE_WRITE], STDERR_FD);
                        npClose(pipedes[PIPE_READ]);
                        // close pipe write-end after dup()
                        npClose(pipedes[PIPE_WRITE]);
                    }
                    else if (isWriteToNumPipe)
                    {
                        npDup(writeToNumPipeWrite, STDOUT_FD);
                        if (isNumPipe2)
                            npDup(writeToNumPipeWrite, STDERR_FD);
                    }

                    //------------------------------------------------------------//
                    // the remaining cases are output to stdout, so nothing to do //
                    //------------------------------------------------------------//

                    // execute command
                    npExec(cmds[i].data);
                    break; // never reach here
                }
                // parnet process
                else
                {
                    if (isReadFromNumPipe)
                        npClose(readFromNumPipeWrite);

                    Proc p(pid, prevRead, STDOUT_FD);
                    if (isPipe)
                        p.writeEnd = pipedes[PIPE_WRITE];
                    else if (isWriteToNumPipe)
                        p.writeEnd = writeToNumPipeWrite;

                    // if (!(procList.insert(std::pair<pid_t, Proc>(pid, p))).second)
                    //     std::cerr << "[ERROR] : cannot insert to procList." << std::endl;
                    procList.push_back(p);

                    if (isNumPipe && !isWriteToNumPipe)
                        numpipeList.push_back(Numpipe(pipedes[PIPE_READ], pipedes[PIPE_WRITE], cmds[i].cnt));
                    // if is not number pipe, close unused pipe write-end
                    if (isPipe && !isNumPipe)
                        npClose(pipedes[PIPE_WRITE]);

                    if (!isLast && isNumPipe)
                    {
                        for (auto &numpipe : numpipeList)
                            numpipe.cnt--;
                    }

                    // if is number pipe, reset the read-end to STDIN
                    // otherwise, set to pip read-end
                    prevRead = (!isPipe && isNumPipe) ? STDIN_FD : pipedes[PIPE_READ];
                    // std::cout << "prevRead = " << prevRead << std::endl;
                    break;
                }
            }
        }

        npWait(procList);
        // for (std::size_t i = 0; i < procList.size(); i++)
        // {
        //     int opt = (procList[i].writeEnd == STDOUT_FD) ? 0 : WNOHANG;
        //     pid_t wpid = waitpid(procList[i].pid, (int *)0, opt);
        //     if (wpid > 0)
        //     {
        //         procList.erase(procList.begin() + i);
        //         i--;
        //     }
        // }
    }
    // std::vector<Cmd> v;
    // npParse(v, "ls -l |2 ls | cat");
    // for (auto elem : v)
    // {
    //     std::cout << elem.cnt << " " << elem.type << " ";
    //     for (auto j : elem.data)
    //         std::cout << j << " ";
    //     std::cout << std::endl;
    // }
    return 0;
}