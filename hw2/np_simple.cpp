// c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>

// c++
#include <iostream>
#include <vector>
#include <string>

using namespace std;

// other include
#include "include/npshell.h"

// define
typedef sockaddr saddr;
typedef sockaddr_in saddr_in;
#define NP1_BACKLOG 32
#define NP1_SOCK_DOMAIN AF_INET
#define NP1_SOCK_TYPE SOCK_STREAM

// global variables
fd_set sock_list;
vector < string > envvar_list;

// functions
vector < string > np2_getdefaultenvvar(char ** envvarp);

int main(int argc, char * argv[], char * envp[]) {
    if (argc != 2) {
        cout << "argc fail" << endl;
        return 1;
    }

    envvar_list = np2_getdefaultenvvar(envp);

    int ssock = socket(NP1_SOCK_DOMAIN, NP1_SOCK_TYPE, 0);
    if (ssock < 0) {
        perror("server socket");
        return 1;
    }
    FD_SET(ssock, & sock_list);

    // set socket option
    int optval = 1;
    socklen_t optval_len = sizeof(optval);
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, & optval, optval_len) < 0) {
        perror("set socket option");
        return 1;
    }

    int sport = stoi(string(argv[1]));
    saddr_in ssaddr;
    ssaddr.sin_family = NP1_SOCK_DOMAIN;
    ssaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ssaddr.sin_port = htons(sport);
    memset(ssaddr.sin_zero, 0, sizeof(ssaddr.sin_zero));

    if (bind(ssock, (const saddr * )( & ssaddr), sizeof(ssaddr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(ssock, NP1_BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    while (true) {
        int csock;
        saddr_in csaddr;
        socklen_t csaddr_len = sizeof(csaddr);
        while ((csock = accept(ssock, (saddr * )( & csaddr), & csaddr_len)) < 0);
        FD_SET(csock, & sock_list);

        pid_t pid;
        while ((pid = fork()) < 0);
        if (pid == 0){ // child
            if (dup2(csock, STDIN_FILENO) < 0)
                perror("dup2 stdin");
            if (dup2(csock, STDOUT_FILENO) < 0)
                perror("dup2 stdout");
            if (dup2(csock, STDERR_FILENO) < 0)
                perror("dup2 stderr");

            Npshell npshell = Npshell(-1, -1, envvar_list);
            npshell.user_list = NULL;
            npshell.sock_list = & sock_list;
            npshell.npshell_list = NULL;

            while (true) {
                cout << "% ";
                string cmd;
                if (!getline(cin, cmd))
                    break;
                if (cmd.size() <= 0)
                    continue;

                for (int i = 0; i < cmd.size(); i++) {
                    if (cmd[i] < 32) {
                        cmd.erase(cmd.begin() + i);
                        i--;
                    }
                }
                if (npshell.np_run(cmd) == 1)
                    break;
            }
            exit(0);
        }
        // parent
        close(csock);
        FD_CLR(csock, & sock_list);
        if (waitpid(pid, NULL, 0) < 0)
            perror("waitpid()");
    }
    close(ssock);
    FD_CLR(ssock, & sock_list);
    return 0;
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