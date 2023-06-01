#pragma once

#include <cstddef>

#include <unistd.h>

class Numpipe
{
  public:
    int read;
    int write;
    int cnt;
    Numpipe(int _read, int _write, int cnt);
}