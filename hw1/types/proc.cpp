#include "proc.hpp"

Proc::Proc(pid_t _pid, int _readEnd, int _writeEnd) : pid(_pid), readEnd(_readEnd), writeEnd(_writeEnd)
{
}