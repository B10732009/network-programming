#include "npshell.h"

vector < np_usrpipe > Npshell::usrpipe_list;

Npshell::Npshell(int _id, int _sock, vector < string > _envvar_list) {
    id = _id;
    sock = _sock;
    envvar_list = _envvar_list;
    pipe_fd[NP_PIPE_READ_END] = NP_RESET;
    pipe_fd[NP_PIPE_WRITE_END] = NP_RESET;
    // prev_read_end = STDIN_FILENO;
    np_setenv("PATH", "bin:.");
}

Npshell::~Npshell() {
    for (int i = 0; i < proc_list.size(); i++) {
        pid_t wpid = waitpid(proc_list[i].pid, NULL, 0);
        np_clean_fd(wpid);
    }
    for (int i = 0; i < usrpipe_list.size(); i++) {
        if (usrpipe_list[i].from_id == id | usrpipe_list[i].to_id == id) {
            usrpipe_list.erase(usrpipe_list.begin() + i);
            i--;
        }
    }
}

vector < np_cmd > Npshell::np_parser(string str) {
    vector < np_cmd > parse = {
        {
            {}, 0, '0'
        }
    };
    string delim = " ";
    string::size_type begin = 0;
    string::size_type end = str.find(delim);
    while (end != string::npos) {
        if (begin < end) {
            string temp = str.substr(begin, end - begin);
            if ( * (temp.begin()) == '|' || * (temp.begin()) == '!') {
                if (temp.size() > 1) {
                    ( * (parse.end() - 1)).pipe_cnt = np_stol(string(temp.begin() + 1, temp.end()));
                    ( * (parse.end() - 1)).pipe_type = * (temp.begin());
                }
                parse.push_back({
                    {},
                    0,
                    '0'
                });
            } else
                ( * (parse.end() - 1)).content.push_back(temp);
        }
        begin = end + delim.size();
        end = str.find(delim, begin);
    }

    if (begin < str.size()) {
        string temp = str.substr(begin);
        if ( * (temp.begin()) == '|' || * (temp.begin()) == '!') {
            if (temp.size() > 1) {
                ( * (parse.end() - 1)).pipe_cnt = np_stol(string(temp.begin() + 1, temp.end()));
                ( * (parse.end() - 1)).pipe_type = * (temp.begin());
            }
        } else
            ( * (parse.end() - 1)).content.push_back(temp);
    }
    return parse;
}

void Npshell::np_setenv(string
    var, string value) {
    if (setenv(var.c_str(), value.c_str(), 1) == 0) {
        envvar_list.push_back(var);
        envvar_list.push_back(value);
    }
}

void Npshell::np_printenv(string
    var) {
    char * env = getenv(var.c_str());
    if (env)
        cout << env << endl;
}

void Npshell::np_exit() {
    exit(0);
}

void Npshell::np_exec(vector < string > cmds) {
    char args[NP_MAX_CMD_ARG][NP_MAX_LINE_LEN];
    char * argp[NP_MAX_CMD_ARG];
    for (int i = 0; i < cmds.size(); i++)
        strcpy(args[i], cmds[i].c_str());
    for (int i = 0; i < cmds.size(); i++)
        argp[i] = args[i];
    argp[cmds.size()] = NULL;

    if (execvp(argp[0], argp) < 0)
        cerr << "Unknown command: [" << argp[0] << "]." << endl;
    exit(1);
}

void Npshell::np_clean_fd(pid_t pid) {
    for (int i = 0; i < proc_list.size(); i++) {
        if (proc_list[i].pid == pid) {
            /*// should not close current using fd
            bool rfound = (proc_list[i].read_end==prev_read_end || 
                proc_list[i].read_end==pipe_fd[NP_PIPE_READ_END] || 
                proc_list[i].read_end==pipe_fd[NP_PIPE_WRITE_END]);
            bool wfound = (proc_list[i].write_end==prev_read_end ||
                proc_list[i].write_end==pipe_fd[NP_PIPE_READ_END] || 
                proc_list[i].write_end==pipe_fd[NP_PIPE_WRITE_END]);

            // should not close fd waiting in the numpipe list
            for(int j=0;j<numpipe_list.size();j++)
            {
                if(proc_list[i].read_end==numpipe_list[j].read_end || 
                    proc_list[i].read_end==numpipe_list[j].write_end)
                    rfound = true;
                if(proc_list[i].write_end==numpipe_list[j].read_end || 
                    proc_list[i].write_end==numpipe_list[j].write_end)
                    wfound = true;
            }*/

            bool rfound = false;
            bool wfound = false;
            if (npshell_list) {
                for (vector < Npshell > ::iterator it = npshell_list -> begin(); it != npshell_list -> end(); it++) {
                    if (it -> id == id)
                        continue;

                    if (it -> np_using_fd(proc_list[i].read_end))
                        rfound = true;
                    if (it -> np_using_fd(proc_list[i].write_end))
                        wfound = true;
                }
            }

            rfound = np_using_fd(proc_list[i].read_end);
            wfound = np_using_fd(proc_list[i].write_end);

            // should not close stdin and stdout
            if (!rfound && proc_list[i].read_end != STDIN_FILENO)
                close(proc_list[i].read_end);
            if (!wfound && proc_list[i].write_end != STDOUT_FILENO)
                close(proc_list[i].write_end);
            break;
        }
    }
}

bool Npshell::np_using_fd(int fd) {
    bool found = false;
    if (prev_read_end == fd)
        return true;
    if (pipe_fd[NP_PIPE_READ_END] == fd || pipe_fd[NP_PIPE_WRITE_END] == fd)
        return true;
    for (vector < np_numpipe > ::iterator it = numpipe_list.begin(); it != numpipe_list.end(); it++) {
        if (it -> read_end == fd || it -> write_end == fd)
            return true;
    }
    if (user_list) {
        for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
            if (it -> sock == fd)
                return true;
        }
    }
    if (sock_list) {
        if (FD_ISSET(fd, sock_list))
            return true;
    }
    for (vector < np_usrpipe > ::iterator it = usrpipe_list.begin(); it != usrpipe_list.end(); it++) {
        if (it -> from_end == fd || it -> to_end == fd)
            return true;
    }
    return false;
}

int Npshell::np_stol(string str) {
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

int Npshell::np_run(string s) {
    np_loadenvvar();

    // number pipe iteration
    for (int i = 0; i < numpipe_list.size(); i++)
        numpipe_list[i].cnt--;

    // parser
    vector < np_cmd > cmds = np_parser(s);

    // built-in functions
    if (cmds.size() == 1 && cmds[0].content.size() == 3 && cmds[0].content[0] == "setenv") {
        np_setenv(cmds[0].content[1], cmds[0].content[2]);
        return 0;
    }
    if (cmds.size() == 1 && cmds[0].content.size() == 2 && cmds[0].content[0] == "printenv") {
        np_printenv(cmds[0].content[1]);
        return 0;
    }
    if (cmds.size() == 1 && cmds[0].content.size() == 1 && cmds[0].content[0] == "exit") {
        // before exit the program, wait all child processes
        for (int i = 0; i < proc_list.size(); i++) {
            pid_t wpid = waitpid(proc_list[i].pid, NULL, 0);
            np_clean_fd(wpid);
        }
        // np_exit();
        return 1;
    }
    // chat built-in functions===============
    if (cmds.size() == 1 && cmds[0].content.size() == 1 && cmds[0].content[0] == "who") {
        np_who();
        return 0;
    }
    if (cmds.size() == 1 && cmds[0].content.size() >= 3 && cmds[0].content[0] == "tell") {
        int tid = np_stol(cmds[0].content[1]);
        string msg = string(s.begin() + cmds[0].content[0].size() + cmds[0].content[1].size() + 2, s.end());
        np_tell(tid, msg);
        return 0;
    }
    if (cmds.size() == 1 && cmds[0].content.size() >= 2 && cmds[0].content[0] == "yell") {
        string msg = string(s.begin() + cmds[0].content[0].size() + 1, s.end());
        np_yell(msg);
        return 0;
    }
    if (cmds.size() == 1 && cmds[0].content.size() >= 2 && cmds[0].content[0] == "name") {
        string name = string(s.begin() + cmds[0].content[0].size() + 1, s.end());
        np_name(name);
        return 0;
    }
    // ======================================

    // initialize previous read end tag to stdin
    prev_read_end = STDIN_FILENO;
    for (int i = 0; i < cmds.size(); i++) {
        bool is_first = (i == 0); // is the first command in the line        
        bool is_last = (i == cmds.size() - 1); // is the last command in the line
        bool is_file_redir = (find(cmds[i].content.begin(), cmds[i].content.end(), ">") != cmds[i].content.end()); // is a file redirection command
        bool is_numpipe_1 = (cmds[i].pipe_type == '|'); // is a number pipe command of type '|'
        bool is_numpipe_2 = (cmds[i].pipe_type == '!'); // is a number pipe command of type '!'
        bool is_numpipe = (is_numpipe_1 || is_numpipe_2); // is a number pipe command
        bool is_ordpipe = !is_numpipe; // is an ordinary pipe command
        bool is_read_from_numpipe = false; // is read form number pipe
        bool is_write_to_numpipe = false; // is write to number pipe
        bool is_pipe; // = (!is_last || is_numpipe);        // pipe() or not
        int read_from_numpipe_write_end;
        int write_to_numpipe_write_end;

        // user pipe flags
        vector < string > ::iterator fit = find_if(cmds[i].content.begin(), cmds[i].content.end(), np_isfromusrpipe);
        vector < string > ::iterator tit = find_if(cmds[i].content.begin(), cmds[i].content.end(), np_istousrpipe);
        bool is_read_from_usrpipe = (fit != cmds[i].content.end());
        bool is_write_to_usrpipe = (tit != cmds[i].content.end());
        int is_read_from_usrpipe_from_id = (is_read_from_usrpipe) ? (stol(string(fit -> begin() + 1, fit -> end()))) : (-1);
        int is_read_from_usrpipe_from_end = -1;
        int is_write_to_usrpipe_to_id = (is_write_to_usrpipe) ? (stol(string(tit -> begin() + 1, tit -> end()))) : (-1);

        // check from user pipe
        if (is_read_from_usrpipe) {
            // check the sender exist
            bool sender_exist = false;
            for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
                if (it -> id == is_read_from_usrpipe_from_id) {
                    sender_exist = true;
                    break;
                }
            }
            if (!sender_exist) // not exist
            {
                np_write(sock, "*** Error: user #" + to_string(is_read_from_usrpipe_from_id) + " does not exist yet. ***\n");
                continue;
            }

            // check the user pipe exist
            vector < np_usrpipe > ::iterator now = usrpipe_list.end();
            for (vector < np_usrpipe > ::iterator it = usrpipe_list.begin(); it != usrpipe_list.end(); it++) {
                if (it -> from_id == is_read_from_usrpipe_from_id && it -> to_id == id) {
                    now = it;
                    break;
                }
            }
            if (now == usrpipe_list.end()) // not exist
            {
                np_write(sock, "*** Error: the pipe #" + to_string(is_read_from_usrpipe_from_id) + "->#" + to_string(id) + " does not exist yet. ***\n");
                continue;
            }

            // connect pipe & broadcast message
            prev_read_end = now -> to_end;
            is_read_from_usrpipe_from_end = now -> from_end;
            vector < User > ::iterator me;
            vector < User > ::iterator sender;
            for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
                if (it -> id == id)
                    me = it;
                if (it -> id == is_read_from_usrpipe_from_id)
                    sender = it;
            }
            string usrpipecmd = "";
            for (vector < string > ::iterator it = cmds[i].content.begin(); it != cmds[i].content.end(); it++)
                usrpipecmd += (it == cmds[i].content.begin()) ? ( * it) : " " + ( * it);
            for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++)
                np_write(it -> sock, "*** " + me -> name + " (#" + to_string(me -> id) + ") just received from " + sender -> name + " (#" + to_string(sender -> id) + ") by '" + s + "' ***\n");
            usrpipe_list.erase(now);
        }

        // check to user pipe
        if (is_write_to_usrpipe) {
            // check receiver exist
            bool receiver_exist = false;
            for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
                if (it -> id == is_write_to_usrpipe_to_id) {
                    receiver_exist = true;
                    break;
                }
            }
            if (!receiver_exist) // not exist
            {
                np_write(sock, "*** Error: user #" + to_string(is_write_to_usrpipe_to_id) + " does not exist yet. ***\n");
                continue;
            }

            // check usrpipe exist
            vector < np_usrpipe > ::iterator now = usrpipe_list.end();
            for (vector < np_usrpipe > ::iterator it = usrpipe_list.begin(); it != usrpipe_list.end(); it++) {
                if (it -> from_id == id && it -> to_id == is_write_to_usrpipe_to_id) {
                    now = it;
                    break;
                }
            }
            if (now != usrpipe_list.end()) // exist
            {
                np_write(sock, "*** Error: the pipe #" + to_string(id) + "->#" + to_string(is_write_to_usrpipe_to_id) + " already exists. ***\n");
                continue;
            }

            // connect pipe & broadcast message
            vector < User > ::iterator me;
            vector < User > ::iterator receiver;
            for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
                if (it -> id == id)
                    me = it;
                if (it -> id == is_write_to_usrpipe_to_id)
                    receiver = it;
            }
            string usrpipecmd = "";
            for (vector < string > ::iterator it = cmds[i].content.begin(); it != cmds[i].content.end(); it++)
                usrpipecmd += (it == cmds[i].content.begin()) ? ( * it) : " " + ( * it);
            for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++)
                np_write(it -> sock, "*** " + me -> name + " (#" + to_string(me -> id) + ") just piped '" + s + "' to " + receiver -> name + " (#" + to_string(receiver -> id) + ") ***\n");
        }

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
        // is_pipe = ((is_ordpipe && !is_last) || (is_numpipe && !is_write_to_numpipe));
        is_pipe = ((is_ordpipe && !is_last) || (is_numpipe && !is_write_to_numpipe) || is_write_to_usrpipe);
        while (is_pipe && pipe(pipe_fd) < 0) {
            // if there is any pid waited by fork(), clean their fd first
            if (!fork_error_wait_list.empty()) {
                for (int j = 0; j < fork_error_wait_list.size(); j++)
                    np_clean_fd(fork_error_wait_list[j]);
                fork_error_wait_list.clear();
                continue;
            }

            // wait former process to clean fd
            int wstatus;
            pid_t wpid = waitpid(-1, & wstatus, 0);
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
                pid_t wpid = waitpid(-1, & wstatus, 0);
                if (WIFEXITED(wstatus))
                    fork_error_wait_list.push_back(wpid);
            } else if (pid == 0) // child process
            {

                dup2(prev_read_end, STDIN_FILENO); // dup read_end to stdin
                if (prev_read_end != STDIN_FILENO)
                    close(prev_read_end); // close that read_end fd
                close(pipe_fd[NP_PIPE_READ_END]); // close unused pipe read_end fd

                if (is_read_from_numpipe)
                    close(read_from_numpipe_write_end); // if is read from number pipe, close the write end of that pipe

                if (is_read_from_usrpipe)
                    if (close(is_read_from_usrpipe_from_end) < 0)
                        perror("dup2() usripipe");

                if (is_file_redir) // file redirection
                {
                    close(pipe_fd[NP_PIPE_WRITE_END]); // no more pipe, so write_end fd is unused
                    int frfd;
                    while ((frfd = open(( * (cmds[i].content.end() - 1)).c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0) {
                        // wait former process to clean fd
                        int wstatus;
                        pid_t wpid = waitpid(-1, & wstatus, 0);
                        if (WIFEXITED(wstatus))
                            np_clean_fd(wpid);
                    }

                    dup2(frfd, STDOUT_FILENO); // dup open file fd to stdout
                    close(frfd); // close open file fd
                    np_exec(vector < string > (cmds[i].content.begin(), cmds[i].content.end() - 2));
                }

                if (is_pipe) // called pipe() this round, so dup the write end of pipe to stdout
                {
                    dup2(pipe_fd[NP_PIPE_WRITE_END], STDOUT_FILENO);
                    if (is_numpipe_2)
                        dup2(pipe_fd[NP_PIPE_WRITE_END], STDERR_FILENO);
                } else if (is_write_to_numpipe) // not called pipe(), but will write to numpipe
                {
                    dup2(write_to_numpipe_write_end, STDOUT_FILENO);
                    if (is_numpipe_2)
                        dup2(write_to_numpipe_write_end, STDERR_FILENO);
                }

                //----------------------------------------------------------
                // the remaining cases are output to stdout, so nothing to do
                //----------------------------------------------------------

                close(pipe_fd[NP_PIPE_WRITE_END]); // close pipe write_end fd

                if (is_read_from_usrpipe)
                    cmds[i].content.erase(fit);
                if (is_write_to_usrpipe)
                    cmds[i].content.erase(tit);

                np_exec(cmds[i].content);
                // break;
            } else // parent process
            {
                if (is_read_from_numpipe)
                    close(read_from_numpipe_write_end); // if is read from number pipe, close the write end of that pipe

                if (is_read_from_usrpipe)
                    close(is_read_from_usrpipe_from_end);

                // push the pid, fds into vector 
                {
                    np_proc p;
                    p.pid = pid;
                    p.read_end = prev_read_end;
                    if (is_pipe)
                        p.write_end = pipe_fd[NP_PIPE_WRITE_END];
                    else if (is_write_to_numpipe)
                        p.write_end = write_to_numpipe_write_end;
                    else
                        p.write_end = STDOUT_FILENO;
                    proc_list.push_back(p);
                }
                // push to numpipe list
                if (is_numpipe && !is_write_to_numpipe) {
                    np_numpipe n;
                    n.read_end = pipe_fd[NP_PIPE_READ_END];
                    n.write_end = pipe_fd[NP_PIPE_WRITE_END];
                    n.cnt = cmds[i].pipe_cnt;
                    numpipe_list.push_back(n);
                }

                // push to usrpipe list
                if (is_write_to_usrpipe) {
                    np_usrpipe u;
                    u.from_id = id;
                    u.from_end = pipe_fd[NP_PIPE_WRITE_END];
                    u.to_id = is_write_to_usrpipe_to_id;
                    u.to_end = pipe_fd[NP_PIPE_READ_END];
                    u.cmd = "@@@";
                    usrpipe_list.push_back(u);
                }

                if (!is_numpipe && !is_write_to_usrpipe) // if(!is_numpipe) 
                    close(pipe_fd[NP_PIPE_WRITE_END]); // if is not number pipe, close pipe write_end fd

                if (is_numpipe)
                    prev_read_end = STDIN_FILENO; // if is number pipe, reset the previos read end
                else
                    prev_read_end = pipe_fd[NP_PIPE_READ_END]; // if is not number pipe, keep the pipe read_end fd for next child to use

                // number pipe iteration
                if (!is_last && is_numpipe) {
                    for (int j = 0; j < numpipe_list.size(); j++)
                        numpipe_list[j].cnt--;
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
        if (proc_list[i].write_end == STDOUT_FILENO || proc_list[i].write_end == sock)
            opt = 0;
        pid_t wpid = waitpid(proc_list[i].pid, NULL, opt);
        np_clean_fd(wpid);
    }

    if ((cmds.end() - 1) -> pipe_type == '|' || (cmds.end() - 1) -> pipe_type == '!')
        return 2;
    return 0;
}

void Npshell::np_who() {
    if (!user_list)
        return;

    vector < User > temp = * user_list;
    sort(temp.begin(), temp.end(), np_cmp);
    np_write(sock, "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
    for (vector < User > ::iterator it = temp.begin(); it != temp.end(); it++) {
        np_write(sock, to_string(it -> id) + "\t" + it -> name + "\t" + it -> addr + ":" + to_string(it -> port));
        if (it -> id == id)
            np_write(sock, "\t<-me");
        np_write(sock, "\n");
    }
}

void Npshell::np_tell(int tid, string msg) {
    if (!user_list)
        return;

    vector < User > ::iterator fnow = user_list -> end();
    vector < User > ::iterator tnow = user_list -> end();
    for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
        if (it -> id == id)
            fnow = it;
        if (it -> id == tid)
            tnow = it;
    }

    if (tnow != user_list -> end()) //found receiver
        np_write(tnow -> sock, "*** " + fnow -> name + " told you ***: " + msg + "\n");
    else // not found receiver
        np_write(sock, "*** Error: user #" + to_string(tid) + " does not exist yet. ***\n");
}

void Npshell::np_yell(string msg) {
    if (!user_list)
        return;

    vector < User > ::iterator now;
    for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
        if (it -> id == id) {
            now = it;
            break;
        }
    }

    for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++)
        np_write(it -> sock, "*** " + now -> name + " yelled ***: " + msg + "\n");
}

void Npshell::np_name(string name) {
    if (!user_list)
        return;

    vector < User > ::iterator now;
    for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
        if (it -> id == id) {
            now = it;
            break;
        }
    }

    vector < User > ::iterator check = user_list -> end();
    for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++) {
        if (it -> name == name) {
            check = it;
            break;
        }
    }

    if (check == user_list -> end()) { // not found same name
        now -> name = name;
        for (vector < User > ::iterator it = user_list -> begin(); it != user_list -> end(); it++)
            np_write(it -> sock, "*** User from " + now -> addr + ":" + to_string(now -> port) + " is named '" + now -> name + "'. ***\n");
    } else // found same name
        np_write(now -> sock, "*** User '" + name + "' already exists. ***\n");

}

void Npshell::np_write(int sock, string str) {
    write(sock, str.c_str(), str.size());
}

void Npshell::np_loadenvvar() {
    for (vector < string > ::iterator it = envvar_list.begin(); it != envvar_list.end(); it += 2)
        setenv(it -> c_str(), (it + 1) -> c_str(), 1);
}

bool np_cmp(User u1, User u2) {
    return (u1.id < u2.id);
}

bool np_isfromusrpipe(string s) {
    return (s.size() > 1 && * (s.begin()) == '<');
}

bool np_istousrpipe(string s) {
    return (s.size() > 1 && * (s.begin()) == '>');
}