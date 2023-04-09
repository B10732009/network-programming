#include "user3.h"
#include <string.h>

User::User(int _id, string _name, int _sock, string _addr, int _port, int _pid) {
    id = _id;
    name = _name;
    sock = _sock;
    addr = _addr;
    port = _port;
    pid = _pid;
}

shmUser init_user(int id, char* name, int sock, char* addr, int port) {
    shmUser u;
    u.id = id;
    u.sock = sock;
    u.port = port;
    strcpy(u.name, name);
    strcpy(u.addr, addr);
    return u;
}