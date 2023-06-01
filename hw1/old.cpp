#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

// c++
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

#define MAX_LINE_LEN 15000 // max number of characters in one line input
#define MAX_CMD_ARG 32     // max number of arguments in one command
#define PIPE_READ_END 0    // read end index of pipe
#define PIPE_WRITE_END 1   // write end index of pipe

// self-defined types
typedef struct _proc { // process struct
    pid_t pid;
    int read_end;
    int write_end;
} proc;

typedef struct _cmd { // command struct
    vector<string> content;
    int pipe_cnt;
    char pipe_type;
} cmd;

typedef struct _numpipe { // number pipe struct
    int read_end;
    int write_end;
    int cnt;
} numpipe;

// global variables
int pipe_fd[2];
int prev_read_end;
vector<proc> proc_list;
vector<numpipe> numpipe_list;
vector<pid_t> fork_error_wait_list;

// functions
vector<cmd> np_parser(string str);
void np_setenv(string var, string value);
void np_printenv(string var);
void np_exit();
void np_exec(vector<string> cmds);
void np_clean_fd(pid_t pid);
int np_stol(string str);

int main() {
    // initialize PATH
    np_setenv("PATH", "bin:.");

    while (true) {
        // read commands
        cout << "% ";
        string s;
        if (!getline(cin, s))
            break;
        if (s.size() <= 0)
            continue;

        // number pipe iteration
        for (int i = 0; i < numpipe_list.size(); i++) {
            numpipe_list[i].cnt--;
        }

        // parser
        vector<cmd> cmds = np_parser(s);

        // built-in functions
        if (cmds.size() == 1 && cmds[0].content.size() == 3 &&
                cmds[0].content[0] == "setenv") {
            np_setenv(cmds[0].content[1], cmds[0].content[2]);
            continue;
        }
        if (cmds.size() == 1 && cmds[0].content.size() == 2 &&
                cmds[0].content[0] == "printenv") {
            np_printenv(cmds[0].content[1]);
            continue;
        }
        if (cmds.size() == 1 && cmds[0].content.size() == 1 &&
                cmds[0].content[0] == "exit") {
            // before exit the program, wait all child processes
            for (int i = 0; i < proc_list.size(); i++) {
                pid_t wpid = waitpid(proc_list[i].pid, NULL, 0);
                np_clean_fd(wpid);
            }
            np_exit();
        }

        // initialize previous read end tag to stdin
        prev_read_end = STDIN_FILENO;
        for (int i = 0; i < cmds.size(); i++) {
            bool is_first = (i == 0);              // is the first command in the line
            bool is_last = (i == cmds.size() - 1); // is the last command in the line
            bool is_file_redir =
                (find(cmds[i].content.begin(), cmds[i].content.end(), ">") !=
                 cmds[i].content.end()); // is a file redirection command
            bool is_numpipe_1 =
                (cmds[i].pipe_type == '|'); // is a number pipe command of type '|'
            bool is_numpipe_2 =
                (cmds[i].pipe_type == '!'); // is a number pipe command of type '!'
            bool is_numpipe =
                (is_numpipe_1 || is_numpipe_2); // is a number pipe command
            bool is_ordpipe = !is_numpipe;      // is an ordinary pipe command
            bool is_read_from_numpipe = false;  // is read form number pipe
            bool is_write_to_numpipe = false;   // is write to number pipe
            bool is_pipe; // = (!is_last || is_numpipe);        // pipe() or not
            int read_from_numpipe_write_end;
            int write_to_numpipe_write_end;

            // if numpipe count down to zero, set it as input
            read_from_numpipe_write_end = -1;
            for (int j = 0; j < numpipe_list.size(); j++) {
                if (numpipe_list[j].cnt <= 0) {
                    is_read_from_numpipe = true;
                    read_from_numpipe_write_end = numpipe_list[j].write_end;
                    prev_read_end = numpipe_list[j].read_end;
                    numpipe_list.erase(numpipe_list.begin() + j);
                    j--;
                }
            }

            write_to_numpipe_write_end = -1;
            for (int j = 0; j < numpipe_list.size(); j++) {
                if (numpipe_list[j].cnt == cmds[i].pipe_cnt) {
                    is_write_to_numpipe = true;
                    write_to_numpipe_write_end = numpipe_list[j].write_end; // j;
                }
            }

            // create pipe
            pipe_fd[0] = -1;
            pipe_fd[1] = -1;
            is_pipe =
                ((is_ordpipe && !is_last) || (is_numpipe && !is_write_to_numpipe));
            while (is_pipe && pipe(pipe_fd) < 0) {
                // if there is any pid waited by fork(), clean their fd first
                if (!fork_error_wait_list.empty()) {
                    for (int j = 0; j < fork_error_wait_list.size(); j++) {
                        np_clean_fd(fork_error_wait_list[j]);
                    }
                    fork_error_wait_list.clear();
                    continue;
                }

                // wait former process to clean fd
                int wstatus;
                pid_t wpid = waitpid(-1, &wstatus, 0);
                if (WIFEXITED(wstatus))
                    np_clean_fd(wpid);
            }

            // fork
            while (1) {
                pid_t pid = fork();
                if (pid == -1) // fork error
                {
                    // wait former process
                    int wstatus;
                    pid_t wpid = waitpid(-1, &wstatus, 0);
                    if (WIFEXITED(wstatus)) {
                        fork_error_wait_list.push_back(wpid);
                    }
                } else if (pid == 0) { // child process
                    // dup read_end to stdin
                    dup2(prev_read_end, STDIN_FILENO);

                    if (prev_read_end != STDIN_FILENO) {
                        // close that read_end fd
                        close(prev_read_end);
                    }

                    // close unused pipe read_end fd
                    close(pipe_fd[PIPE_READ_END]);

                    if (is_read_from_numpipe) {
                        // if is read from number pipe, close the write end of that pipe
                        close(read_from_numpipe_write_end);
                    }

                    if (is_file_redir) { // file redirection
                        // no more pipe, so write_end fd is unused
                        close(pipe_fd[PIPE_WRITE_END]);

                        int frfd;
                        while ((frfd = open((*(cmds[i].content.end() - 1)).c_str(),
                                            O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
                            // wait former process to clean fd
                            int wstatus;
                            pid_t wpid = waitpid(-1, &wstatus, 0);
                            if (WIFEXITED(wstatus))
                                np_clean_fd(wpid);
                        }

                        // dup open file fd to stdout
                        dup2(frfd, STDOUT_FILENO);

                        // close open file fd
                        close(frfd);

                        np_exec(vector<string>(cmds[i].content.begin(),
                                               cmds[i].content.end() - 2));
                    }

                    if (is_pipe) { // called pipe() this round, so dup the write end of
                        // pipe to stdout
                        dup2(pipe_fd[PIPE_WRITE_END], STDOUT_FILENO);
                        if (is_numpipe_2) {
                            dup2(pipe_fd[PIPE_WRITE_END], STDERR_FILENO);
                        }
                    } else if (is_write_to_numpipe) { // not called pipe(), but will write
                        // to numpipe
                        dup2(write_to_numpipe_write_end, STDOUT_FILENO);
                        if (is_numpipe_2) {
                            dup2(write_to_numpipe_write_end, STDERR_FILENO);
                        }
                    }

                    //----------------------------------------------------------
                    // the remaining cases are output to stdout, so nothing to do
                    //----------------------------------------------------------

                    // close pipe write_end fd
                    close(pipe_fd[PIPE_WRITE_END]);
                    np_exec(cmds[i].content);
                    // break;
                } else { // parent process
                    if (is_read_from_numpipe) {
                        // if is read from number pipe, close the write end of that pipe
                        close(read_from_numpipe_write_end);
                    }

                    // push the pid, fds into vector
                    {
                        proc p;
                        p.pid = pid;
                        p.read_end = prev_read_end;
                        if (is_pipe) {
                            p.write_end = pipe_fd[PIPE_WRITE_END];
                        } else if (is_write_to_numpipe) {
                            p.write_end = write_to_numpipe_write_end;
                        } else {
                            p.write_end = STDOUT_FILENO;
                        }
                        proc_list.push_back(p);
                    }

                    // push to numpipe list
                    if (is_numpipe && !is_write_to_numpipe) {
                        numpipe n;
                        n.read_end = pipe_fd[PIPE_READ_END];
                        n.write_end = pipe_fd[PIPE_WRITE_END];
                        n.cnt = cmds[i].pipe_cnt;
                        numpipe_list.push_back(n);
                    }

                    if (!is_numpipe) {
                        // if is not number pipe, close pipe write_end fd
                        close(pipe_fd[PIPE_WRITE_END]);
                    }

                    if (is_numpipe) {
                        // if is number pipe, reset the previos read end
                        prev_read_end = STDIN_FILENO;
                    } else {
                        // if is not number pipe, keep the pipe read_end fd for next child
                        // to use
                        prev_read_end = pipe_fd[PIPE_READ_END];
                    }

                    // number pipe iteration
                    if (!is_last && is_numpipe) {
                        for (int j = 0; j < numpipe_list.size(); j++) {
                            numpipe_list[j].cnt--;
                        }
                    }

                    break;
                }
            }
        }

        // wait child processes and clean fd
        for (int i = 0; i < proc_list.size(); i++) {
            // if the process is output to stdout, wait until it exit
            // if not, just check if it exited
            int opt = WNOHANG;
            if (proc_list[i].write_end == STDOUT_FILENO) {
                opt = 0;
            }
            pid_t wpid = waitpid(proc_list[i].pid, NULL, opt);
            np_clean_fd(wpid);
        }
    }
    return 0;
}

vector<cmd> np_parser(string str) {
    vector<cmd> parse = {{{}, 0, '0'}};
    string delim = " ";
    string::size_type begin = 0;
    string::size_type end = str.find(delim);
    while (end != string::npos) {
        if (begin < end) {
            string temp = str.substr(begin, end - begin);
            if (*(temp.begin()) == '|' || *(temp.begin()) == '!') {
                if (temp.size() > 1) {
                    (*(parse.end() - 1)).pipe_cnt =
                        np_stol(string(temp.begin() + 1, temp.end()));
                    (*(parse.end() - 1)).pipe_type = *(temp.begin());
                }
                parse.push_back({{}, 0, '0'});
            } else {
                (*(parse.end() - 1)).content.push_back(temp);
            }
        }
        begin = end + delim.size();
        end = str.find(delim, begin);
    }

    if (begin < str.size()) {
        string temp = str.substr(begin);
        if (*(temp.begin()) == '|' || *(temp.begin()) == '!') {
            if (temp.size() > 1) {
                (*(parse.end() - 1)).pipe_cnt =
                    np_stol(string(temp.begin() + 1, temp.end()));
                (*(parse.end() - 1)).pipe_type = *(temp.begin());
            }
        } else {
            (*(parse.end() - 1)).content.push_back(temp);
        }
    }
    return parse;
}

void np_setenv(string var, string value) {
    if (setenv(var.c_str(), value.c_str(), 1) < 0)
        perror("setenv error: ");
}

void np_printenv(string var) {
    char *env = getenv(var.c_str());
    if (env)
        cout << env << endl;
}

void np_exit() {
    exit(0);
}

void np_exec(vector<string> cmds) {
    char args[MAX_CMD_ARG][MAX_LINE_LEN];
    char *argp[MAX_CMD_ARG];
    for (int i = 0; i < cmds.size(); i++)
        strcpy(args[i], cmds[i].c_str());
    for (int i = 0; i < cmds.size(); i++)
        argp[i] = args[i];
    argp[cmds.size()] = NULL;

    if (execvp(argp[0], argp) < 0)
        cerr << "Unknown command: [" << argp[0] << "]." << endl;
    exit(1);
}

void np_clean_fd(pid_t pid) {
    for (int i = 0; i < proc_list.size(); i++) {
        if (proc_list[i].pid == pid) {
            // should not close current using fd
            bool rfound = (proc_list[i].read_end == prev_read_end ||
                           proc_list[i].read_end == pipe_fd[PIPE_READ_END] ||
                           proc_list[i].read_end == pipe_fd[PIPE_WRITE_END]);
            bool wfound = (proc_list[i].write_end == prev_read_end ||
                           proc_list[i].write_end == pipe_fd[PIPE_READ_END] ||
                           proc_list[i].write_end == pipe_fd[PIPE_WRITE_END]);

            // should not close fd waiting in the numpipe list
            for (int j = 0; j < numpipe_list.size(); j++) {
                if (proc_list[i].read_end == numpipe_list[j].read_end ||
                        proc_list[i].read_end == numpipe_list[j].write_end)
                    rfound = true;
                if (proc_list[i].write_end == numpipe_list[j].read_end ||
                        proc_list[i].write_end == numpipe_list[j].write_end)
                    wfound = true;
            }

            // should not close stdin and stdout
            if (!rfound && proc_list[i].read_end != STDIN_FILENO)
                close(proc_list[i].read_end);
            if (!wfound && proc_list[i].write_end != STDOUT_FILENO)
                close(proc_list[i].write_end);
            break;
        }
    }
}

int np_stol(string str) {
    int sum = 0;
    int temp = 0;
    for (int i = 0; i < str.size(); i++) {
        if (str[i] == '+') {
            sum += temp;
            temp = 0;
        } else
            temp = temp * 10 + (int)(str[i] - '0');
    }
    sum += temp;
    return sum;
}