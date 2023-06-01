#pragma once

#include <cstddef>

#include <unistd.h>

class Proc
{
  public:
    pid_t pid;
    int readEnd;
    int writeEnd;
    Proc() = delete;
    Proc(pid_t _pid, int _readEnd, int _writeEnd);
};