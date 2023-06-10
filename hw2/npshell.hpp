#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class Proc // process structure, recording process infos
{
  public:
    pid_t pid;    // process ID
    int readEnd;  // read-end FD
    int writeEnd; // write-end FD
    Proc() = delete;
    Proc(pid_t _pid, int _readEnd, int _writeEnd);
};

class Cmd // command structure, recording command infos
{
  public:
    std::vector<std::string> data; // command content
    int cnt;                       // count (for number pipe)
    char type;                     // type ('0' -> ordinary pipe, '|' and '!' -> number pipe)
    Cmd() = delete;
    Cmd(std::vector<std::string> _data, int _cnt, char _type);
};

class Numpipe // number pipe structure, recording number pipe infos
{
  public:
    int read;  // read-end FD
    int write; // wirte-end FD
    int cnt;   // count
    Numpipe() = delete;
    Numpipe(int _read, int _write, int _cnt);
};

class NpShell
{
  private:
    std::vector<Proc> procList;
    std::vector<Numpipe> numpipeList;

    void npParse(std::vector<Cmd> &cmds, std::string str);
    void npSetenv(std::string var, std::string value);
    void npPrintenv(std::string var);
    void npExec(std::vector<std::string> cmds);
    void npExit();
    void npDup(int fd1, int fd2);
    void npClose(int fd);
    bool npWait(std::vector<Proc> &procs);
    void npWrite(int fd, std::string str);

  public:
    NpShell();
    ~NpShell();
    bool npRun(std::string str);
};