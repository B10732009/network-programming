// c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

// c++
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

using namespace std;

// other include
#include "include/user.h"

#include "include/npshell.h"

// define
typedef sockaddr saddr;
typedef sockaddr_in saddr_in;
#define NP2_BACKLOG 0
#define NP2_SOCK_DOMAIN AF_INET
#define NP2_SOCK_TYPE SOCK_STREAM
#define NP2_RESET - 1
#define NP2_MAX_USER 30

// global variables
vector < int > fd_list;
vector < bool > id_list(NP2_MAX_USER + 1, true);
vector < User > user_list;
vector < Npshell > npshell_list;
fd_set sock_list;
int max_sock = 2;
vector < string > envvar_list;

// functions
void np2_login(int id);
void np2_logout(int id);
int np2_getid();
vector < string > np2_getdefaultenvvar(char ** envvarp);
bool np2_usercmp(User u1, User u2);
string np2_read(int sock);
void np2_write(int sock, string str);

int main(int argc, char * argv[], char * envp[]) {
    if (argc != 2) {
        cout << "argc fail" << endl;
        return 1;
    }

    envvar_list = np2_getdefaultenvvar(envp);

    int ssock = socket(NP2_SOCK_DOMAIN, NP2_SOCK_TYPE, 0);
    if (ssock < 0) {
        perror("socket()");
        return 1;
    }

    int optval = 1;
    socklen_t optval_len = sizeof(optval);
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, & optval, optval_len) < 0) {
        perror("setsockopt()");
        return 1;
    }

    int sport = stoi(string(argv[1]));
    saddr_in ssaddr;
    ssaddr.sin_family = NP2_SOCK_DOMAIN;
    ssaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ssaddr.sin_port = htons(sport);
    memset(ssaddr.sin_zero, 0, sizeof(ssaddr.sin_zero));

    if (bind(ssock, (const saddr * )( & ssaddr), sizeof(ssaddr)) < 0) {
        perror("bind()");
        return 1;
    }

    if (listen(ssock, NP2_BACKLOG) < 0) {
        perror("listen()");
        return 1;
    }

    FD_SET(ssock, & sock_list);
    if (ssock > max_sock)
        max_sock = ssock;

    while (true) {
        fd_set temp_list;
        memcpy( & temp_list, & sock_list, sizeof(sock_list));
        while (select(max_sock + 1, & temp_list, NULL, NULL, NULL) < 0) {
            perror("select()");
            exit(1);
        }

        if (FD_ISSET(ssock, & temp_list)) { // is from listening socket
        
            int csock;
            saddr_in csaddr;
            socklen_t csaddr_len = sizeof(csaddr);
            csock = accept(ssock, (saddr * )( & csaddr), & csaddr_len);
            FD_SET(csock, & sock_list);
            if (csock > max_sock)
                max_sock = csock;

            int cid = np2_getid();
            if (cid != NP2_RESET) {
                User user(cid, "(no name)", csock, inet_ntoa(csaddr.sin_addr), ntohs(csaddr.sin_port));
                user_list.push_back(user);

                Npshell npshell(cid, csock, envvar_list);
                npshell.sock_list = & sock_list;
                npshell.user_list = & user_list;
                npshell_list.push_back(npshell);

                np2_login(cid);
                np2_write(csock, "% ");
            }
            continue;
        }

        for (int i = 0; i < user_list.size(); i++) {
            if (!FD_ISSET(user_list[i].sock, & temp_list))
                continue;

            string cmd = np2_read(user_list[i].sock);
            if (dup2(user_list[i].sock, STDOUT_FILENO) < 0)
                perror("dup2()");
            if (dup2(user_list[i].sock, STDERR_FILENO) < 0)
                perror("dup2()");

            vector < Npshell > ::iterator nit;
            for (nit = npshell_list.begin(); nit != npshell_list.end(); nit++) {
                if (nit -> id == user_list[i].id)
                    break;
            }

            int check = nit -> np_run(cmd);
            if (check == 0)
            ;
            np2_write(user_list[i].sock, "% ");
            if (check == 1) {
                np2_logout(user_list[i].id);
                if (dup2(STDIN_FILENO, STDOUT_FILENO) < 0)
                    perror("np close");
                if (dup2(STDIN_FILENO, STDERR_FILENO) < 0)
                    perror("np close");
                if (close(user_list[i].sock) < 0)
                    perror("logout");

                FD_CLR(user_list[i].sock, & sock_list);
                id_list[user_list[i].id] = true;
                user_list.erase(user_list.begin() + i);
                i--;
                npshell_list.erase(nit);
            }
        }
    }
    close(ssock);
    FD_CLR(ssock, & sock_list);
    return 0;
}

void np2_login(int id) {
    vector < User > ::iterator now;
    for (vector < User > ::iterator it = user_list.begin(); it != user_list.end(); it++) {
        if (it -> id == id) {
            now = it;
            break;
        }
    }

    np2_write(now -> sock, "****************************************\n");
    np2_write(now -> sock, "** Welcome to the information server. **\n");
    np2_write(now -> sock, "****************************************\n");

    for (vector < User > ::iterator it = user_list.begin(); it != user_list.end(); it++)
        np2_write(it -> sock, "*** User '" + now -> name + "' entered from " +
            now -> addr + ":" + to_string(now -> port) + ". ***\n");
}

void np2_logout(int id) {
    vector < User > ::iterator now;
    for (vector < User > ::iterator it = user_list.begin(); it != user_list.end(); it++) {
        if (it -> id == id) {
            now = it;
            break;
        }
    }

    for (vector < User > ::iterator it = user_list.begin(); it != user_list.end(); it++) {
        if (it != now)
            np2_write(it -> sock, "*** User '" + now -> name + "' left. ***\n");
    }
}

int np2_getid() {
    for (int i = 1; i < id_list.size(); i++) {
        if (id_list[i]) {
            id_list[i] = false;
            return i;
        }
    }

    return NP2_RESET;
}

vector < string > np2_getdefaultenvvar(char ** envvarp) {
    vector < string > _envvar_list;
    if (!envvarp)
        return _envvar_list;

    for (char ** p = envvarp;* p != NULL; p++) {
        string env = * p;
        int dloc = env.size();
        for (int i = 0; i < env.size(); i++) {
            if (env[i] == '=') {
                dloc = i;
                break;
            }
        }

        if (dloc != env.size()) {
            _envvar_list.push_back(string(env.begin(), env.begin() + dloc));
            _envvar_list.push_back(string(env.begin() + dloc + 1, env.end()));
        }
    }
    return _envvar_list;
}

bool np2_usercmp(User u1, User u2) {
    return (u1.id < u2.id);
}

string np2_read(int sock) {
    char buf[15000] = {
        0
    };
    read(sock, buf, sizeof(buf) / sizeof(char));
    string str = buf;
    for (int i = 0; i < str.size(); i++) {
        if (str[i] < 32) {
            str.erase(str.begin() + i);
            i--;
        }
    }
    return str;
}

void np2_write(int sock, string str) {
    write(sock, str.c_str(), str.size());
}