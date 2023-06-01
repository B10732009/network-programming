#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <unistd.h>

class Cmd
{
  public:
    std::vector<std::string> data;
    int cnt;
    char type;
    Cmd() = delete;
    Cmd(std::vector<std::string> _data, int _cnt, char _type);
};