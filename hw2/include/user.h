#ifndef USER_H
#define USER_H

#include <iostream>
#include <vector>
#include <string>

using namespace std;

class User {
public:
    int id;
    string name;
    int sock;
    string addr;
    int port;

    User(int _id, string _name, int _sock, string _addr, int _port);
};

#endif