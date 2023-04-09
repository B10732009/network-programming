#include "user.h"

User::User(int _id, string _name, int _sock, string _addr, int _port) {
    id = _id;
    name = _name;
    sock = _sock;
    addr = _addr;
    port = _port;
}