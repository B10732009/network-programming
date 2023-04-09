// c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>

// c++
#include <iostream>
#include <vector>
#include <string>

using namespace std;

// other include
#include "include/npshell3.h"
#include "include/user3.h"

// define
typedef sockaddr saddr;
typedef sockaddr_in saddr_in;
#define NP1_BACKLOG 0
#define NP1_SOCK_DOMAIN AF_INET
#define NP1_SOCK_TYPE SOCK_STREAM

// global variables
fd_set sock_list;
vector < string > envvar_list;
vector < User > user_list;
vector < bool > id_list(31, true);

// shm
shmUser * shm_user_list_pointer;
key_t shm_user_list_key = 9000;
int * shm_user_list_size_pointer;
key_t shm_user_list_size_key = 9001;
fd_set * shm_sock_list_pointer;
key_t shm_sock_list_key = 9002;
fd_set * shm_numpipe_list_pointer;
key_t shm_numpipe_list_key = 9003;
bool * shm_id_list_pointer;
key_t shm_id_list_key = 9004;
char * shm_msg_pointer;
key_t shm_msg_key = 9005;
pid_t * shm_pid_pointer;
key_t shm_pid_key = 9006;

int shm_user_list_id;
int shm_user_list_size_id;
int shm_sock_list_id;
int shm_numpipe_list_id;
int shm_id_list_id;
int shm_msg_id;
int shm_pid_id;

void get_shm_user_list();
void set_shm_user_list();

// functions
vector < string > np2_getdefaultenvvar(char ** envvarp);
void sighandler(int signum);
int np2_getid();
void np2_login(int id);
void np2_logout(int id);
void np2_write(int sock, string str);

// signal handler
void sighan(int signum) {
    cout << string(shm_msg_pointer);
}

void sighan2(int signum) {
    // detach shared memory
    if (shmdt(shm_user_list_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_user_list_size_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_sock_list_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_numpipe_list_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_id_list_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_msg_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_pid_pointer) < 0)
        perror("shmdt()");
    // delete shared memory
    if (shmctl(shm_user_list_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_user_list_size_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_sock_list_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_numpipe_list_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_id_list_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_msg_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_pid_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    exit(1);
}

int main(int argc, char * argv[], char * envp[]) {
    if (argc != 2) {
        cout << "argc fail" << endl;
        return 1;
    }

    signal(SIGUSR1, & sighan);
    signal(SIGINT, & sighan2);

    // shm
    // shm allocation
    shm_user_list_id = shmget(shm_user_list_key, sizeof(shmUser) * 30, IPC_CREAT | 0666);
    if (shm_user_list_id < 0) {
        perror("shmget()1");
        return 1;
    }
    shm_user_list_size_id = shmget(shm_user_list_size_key, sizeof(int), IPC_CREAT | 0666);
    if (shm_user_list_size_id < 0) {
        perror("shmget()2");
        return 1;
    }
    shm_sock_list_id = shmget(shm_sock_list_key, sizeof(fd_set), IPC_CREAT | 0666);
    if (shm_sock_list_id < 0) {
        perror("shmget()3");
        return 1;
    }
    shm_numpipe_list_id = shmget(shm_numpipe_list_key, sizeof(fd_set) * 30, IPC_CREAT | 0666);
    if (shm_numpipe_list_id < 0) {
        perror("shmget()4");
        return 1;
    }
    shm_id_list_id = shmget(shm_id_list_key, sizeof(bool) * 31, IPC_CREAT | 0666);
    if (shm_id_list_id < 0) {
        perror("shmget()5");
        return 1;
    }
    shm_msg_id = shmget(shm_msg_key, sizeof(char) * 256, IPC_CREAT | 0666);
    if (shm_msg_id < 0) {
        perror("shmget()6");
        return 1;
    }
    shm_pid_id = shmget(shm_pid_key, sizeof(pid_t), IPC_CREAT | 0666);
    if (shm_pid_id < 0) {
        perror("shmget()7");
        return 1;
    }
    // shm attachment
    shm_user_list_pointer = (shmUser * ) shmat(shm_user_list_id, NULL, 0);
    if (shm_user_list_pointer == (shmUser * ) - 1) {
        perror("shmat()");
        return 1;
    }
    shm_user_list_size_pointer = (int * ) shmat(shm_user_list_size_id, NULL, 0);
    if (shm_user_list_size_pointer == (int * ) - 1) {
        perror("shmat()");
        return 1;
    }
    shm_sock_list_pointer = (fd_set * ) shmat(shm_sock_list_id, NULL, 0);
    if (shm_sock_list_pointer == (fd_set * ) - 1) {
        perror("shmat()");
        return 1;
    }
    shm_numpipe_list_pointer = (fd_set * ) shmat(shm_numpipe_list_id, NULL, 0);
    if (shm_numpipe_list_pointer == (fd_set * ) - 1) {
        perror("shmat()");
        return 1;
    }
    shm_id_list_pointer = (bool * ) shmat(shm_id_list_id, NULL, 0);
    if (shm_id_list_pointer == (bool * ) - 1) {
        perror("shmat()");
        return 1;
    }
    shm_msg_pointer = (char * ) shmat(shm_msg_id, NULL, 0);
    if (shm_msg_pointer == (char * ) - 1) {
        perror("shmat()");
        return 1;
    }
    shm_pid_pointer = (pid_t * ) shmat(shm_pid_id, NULL, 0);
    if (shm_pid_pointer == (pid_t * ) - 1) {
        perror("shmat()");
        return 1;
    }

    ( * shm_user_list_size_pointer) = 0;
    for (int i = 0; i < 31; i++)
        shm_id_list_pointer[i] = true;
    FD_ZERO(shm_sock_list_pointer);
    for (int i = 0; i < 31; i++)
        FD_ZERO(shm_numpipe_list_pointer + i);

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
        FD_SET(csock, shm_sock_list_pointer);

        int cid = np2_getid();
        if (cid < 0)
            continue;
        cout << cid << endl;

        pid_t pid;
        while ((pid = fork()) < 0);
        if (pid == 0) { // child
            if (dup2(csock, STDIN_FILENO) < 0)
                perror("dup2 stdin");
            if (dup2(csock, STDOUT_FILENO) < 0)
                perror("dup2 stdout");
            if (dup2(csock, STDERR_FILENO) < 0)
                perror("dup2 stderr");

            // create npshell
            Npshell npshell = Npshell(cid, csock, envvar_list);
            npshell.user_list = & user_list;
            npshell.sock_list = shm_sock_list_pointer;
            npshell.npshell_list = NULL;
            npshell.shm_user_list_pointer = shm_user_list_pointer;
            npshell.shm_user_list_size_pointer = shm_user_list_size_pointer;
            npshell.shm_numpipe_list_pointer = shm_numpipe_list_pointer;
            npshell.shm_msg_pointer = shm_msg_pointer;

            // login messages
            np2_login(cid);

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
                if (npshell.np_run(cmd) == 1) // logout
                {
                    // logout messages
                    np2_logout(cid);
                    // detach shared memory
                    /*if(shmdt(shm_user_list_pointer)<0)
                        perror("shmdt()");
                    if(shmdt(shm_user_list_size_pointer)<0)
                        perror("shmdt()");
                    if(shmdt(shm_sock_list_pointer)<0)
                        perror("shmdt()");
                    if(shmdt(shm_numpipe_list_pointer)<0)
                        perror("shmdt()");
                    if(shmdt(shm_msg_pointer)<0)
                        perror("shmdt()");*/
                    // delete user
                    get_shm_user_list();
                    for (int i = 0; i < user_list.size(); i++) {
                        if (user_list[i].id == cid) {
                            user_list.erase(user_list.begin() + i);
                            break;
                        }
                    }
                    set_shm_user_list();
                    // return id
                    shm_id_list_pointer[cid] = true;
                    break;
                }

            }
            close(csock);
            exit(0);
        }
        // parent
        // add user
        get_shm_user_list();
        user_list.push_back(User(cid, "(no name)", csock, inet_ntoa(csaddr.sin_addr), ntohs(csaddr.sin_port), pid));
        set_shm_user_list();

        close(csock);
        //FD_CLR(csock, &sock_list);
        //if(waitpid(pid, NULL, 0)<0)
        //perror("waitpid()");
    }
    close(ssock);
    FD_CLR(ssock, shm_sock_list_pointer);
    // detach shared memory
    if (shmdt(shm_user_list_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_user_list_size_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_sock_list_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_numpipe_list_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_msg_pointer) < 0)
        perror("shmdt()");
    if (shmdt(shm_pid_pointer) < 0)
        perror("shmdt()");
    // delete shared memory
    if (shmctl(shm_user_list_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_user_list_size_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_sock_list_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_numpipe_list_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_msg_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
    if (shmctl(shm_pid_id, IPC_RMID, 0) < 0)
        perror("shmctl()");
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

void get_shm_user_list() {
    if (!shm_user_list_pointer)
        return;
    user_list.clear();
    for (int i = 0; i < ( * shm_user_list_size_pointer); i++)
        user_list.push_back(User(
            shm_user_list_pointer[i].id,
            shm_user_list_pointer[i].name,
            shm_user_list_pointer[i].sock,
            shm_user_list_pointer[i].addr,
            shm_user_list_pointer[i].port,
            shm_user_list_pointer[i].pid));; //, cout<<shm_user_list_pointer[i].sock<<endl;
}

void set_shm_user_list() {
    if (!shm_user_list_pointer)
        return;

    ( * shm_user_list_size_pointer) = user_list.size();
    for (int i = 0; i < user_list.size(); i++) {
        shmUser u;
        u.id = user_list[i].id;
        u.sock = user_list[i].sock;
        u.port = user_list[i].port;
        u.pid = user_list[i].pid;
        strcpy(u.name, user_list[i].name.c_str());
        strcpy(u.addr, user_list[i].addr.c_str());
        memcpy(shm_user_list_pointer + i, & u, sizeof(shmUser));
    }
}

int np2_getid() {
    for (int i = 1; i < 31; i++) {
        if (shm_id_list_pointer[i]) {
            shm_id_list_pointer[i] = false;
            return i;

        }
    }

    return -1;
}

void np2_login(int id) {
    get_shm_user_list();
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
    strcpy(shm_msg_pointer, string(
        "*** User '" + now -> name + "' entered from " +
        now -> addr + ":" + to_string(now -> port) + ". ***\n").c_str());
    for (vector < User > ::iterator it = user_list.begin(); it != user_list.end(); it++)
        //np2_write(it->sock, "*** User '"+now->name+"' entered from "
        //+now->addr+":"+to_string(now->port)+". ***\n");
        kill(it -> pid, SIGUSR1);
}

void np2_logout(int id) {
    get_shm_user_list();
    vector < User > ::iterator now;
    for (vector < User > ::iterator it = user_list.begin(); it != user_list.end(); it++) {
        if (it -> id == id) {
            now = it;
            break;
        }
    }

    strcpy(shm_msg_pointer, string("*** User '" + now -> name + "' left. ***\n").c_str());
    for (vector < User > ::iterator it = user_list.begin(); it != user_list.end(); it++) {
        if (it != now)
            //np2_write(it->sock, "*** User '"+now->name+"' left. ***\n");
            kill(it -> pid, SIGUSR1);
    }
}

void np2_write(int sock, string str) {
    write(sock, str.c_str(), str.size());
}