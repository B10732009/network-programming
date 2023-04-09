#ifndef USER3_H
#define USER3_H

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
    int pid;

    User(int _id, string _name, int _sock, string _addr, int _port, int pid);
};

typedef struct {
    int id;
    char name[100];
    int sock;
    char addr[100];
    int port;
    int pid;
} shmUser;

shmUser init_user(int id, char* name, int sock, char* addr, int port);

#endif