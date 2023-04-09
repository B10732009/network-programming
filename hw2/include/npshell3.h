#ifndef NPSHELL3_H
#define NPSHELL3_H

// c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>

// c++
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

#include "user3.h"

#define NP_MAX_LINE_LEN 15000 // max number of characters in one line input
#define NP_MAX_CMD_ARG 32 // max number of arguments in one command
#define NP_PIPE_READ_END 0 // read end index of pipe
#define NP_PIPE_WRITE_END 1 // write end index of pipe

#define NP_RESET - 1

// self-defined types
typedef struct _np_proc // process struct
{
    pid_t pid;
    int read_end;
    int write_end;
}
np_proc;

typedef struct _np_cmd // command struct
{
    vector < string > content;
    int pipe_cnt;
    char pipe_type;
}
np_cmd;

typedef struct _np_numpipe // number pipe struct
{
    int read_end;
    int write_end;
    int cnt;
}
np_numpipe;

typedef struct _np_usrpipe {
    int from_end;
    int to_end;
    int from_id;
    int to_id;
    string cmd;
}
np_usrpipe;

class Npshell {
    private: int pipe_fd[2];
    int prev_read_end;
    vector < np_proc > proc_list;
    vector < np_numpipe > numpipe_list;
    vector < pid_t > fork_error_wait_list;
    vector < string > envvar_list;
    static vector < np_usrpipe > usrpipe_list;

    vector < np_cmd > np_parser(string str);
    void np_setenv(string
        var, string value);
    void np_printenv(string
        var);
    void np_exit();
    void np_exec(vector < string > cmds);
    void np_clean_fd(pid_t pid);
    int np_stol(string str);

    // chat built-in functions
    void np_who();
    void np_tell(int tid, string msg);
    void np_yell(string msg);
    void np_name(string name);
    void np_write(int sock, string str);
    void np_loadenvvar();

    // shm
    void get_shm_user_list();
    void set_shm_user_list();

    public: fd_set * sock_list;
    vector < User > * user_list;
    shmUser * shm_user_list_pointer;
    int * shm_user_list_size_pointer;
    fd_set * shm_numpipe_list_pointer;
    char * shm_msg_pointer;
    int * shm_msg_to_pointer;
    vector < Npshell > * npshell_list;
    int id;
    int sock;
    Npshell();
    Npshell(int _id, int _sock, vector < string > _envvar_list);
    ~Npshell();
    bool np_using_fd(int fd);
    int np_run(string s);
};

bool np_cmp(User u1, User u2);
bool np_isfromusrpipe(string s);
bool np_istousrpipe(string s);

// npshell for shared memory
typedef struct {
    fd_set shm_np_numpipe_list;
}
shmNpshell;

#endif