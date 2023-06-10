#include "npshell.hpp"

#define MAX_CMD_ARG 32    // max number of argument of a single command
#define MAX_CMD_LEN 15000 // max length of a single command

#define STDIN_FD 0  // stdin FD
#define STDOUT_FD 1 // stdout FD
#define STDERR_FD 2 // stderr FD

#define PIPE_READ 0  // pipe read-end
#define PIPE_WRITE 1 // pipe write-end

Proc::Proc(pid_t _pid, int _readEnd, int _writeEnd) : pid(_pid), readEnd(_readEnd), writeEnd(_writeEnd)
{
}

Cmd::Cmd(std::vector<std::string> _data, int _cnt, char _type) : data(_data), cnt(_cnt), type(_type)
{
}

Numpipe::Numpipe(int _read, int _write, int _cnt) : read(_read), write(_write), cnt(_cnt)
{
}

bool NpShell::npRun(std::string str)
{
    // parse the input string
    std::vector<Cmd> cmds;
    npParse(cmds, str);

    // decrement number pipe list count
    for (auto &numpipe : numpipeList)
        numpipe.cnt--;

    // built-in functions
    if (cmds.size() == 1 && cmds[0].data.size() == 3 && cmds[0].data[0] == "setenv")
    {
        npSetenv(cmds[0].data[1], cmds[0].data[2]);
        return true;
    }
    else if (cmds.size() == 1 && cmds[0].data.size() == 2 && cmds[0].data[0] == "printenv")
    {
        npPrintenv(cmds[0].data[1]);
        return true;
    }
    else if (cmds.size() == 1 && cmds[0].data.size() == 1 && cmds[0].data[0] == "exit")
    {
        npWait(procList);
        // npExit();
        return false;
    }

    // read-end FD of last round, initially set to STDIN
    int prevRead = STDIN_FD;

    for (std::size_t i = 0; i < cmds.size(); i++)
    {
        // bool isFirst = (i == 0); // not used                                                         // is first command
        bool isLast = (i == cmds.size() - 1);                                                           // is last command
        bool isFile = (std::find(cmds[i].data.begin(), cmds[i].data.end(), ">") != cmds[i].data.end()); // is file redirection
        bool isNumPipe1 = (cmds[i].type == '|');                                                        // is number pipe (type1)
        bool isNumPipe2 = (cmds[i].type == '!');                                                        // is number pipe (type2)
        bool isNumPipe = isNumPipe1 || isNumPipe2;                                                      // is number pipe
        bool isOrdPipe = !isNumPipe;                                                                    // is ordinary pipe
        bool isPipe;                                                                                    // is to create new pipe (call()) or not in this round

        bool isReadFromNumPipe = false;
        bool isWriteToNumPipe = false;
        int readFromNumPipeWrite = -1; // write-end of the number pipe reading from
        int writeToNumPipeWrite = -1;  // write-end of the number pipe writing to

        // check if read from number pipe this round
        for (std::size_t j = 0; j < numpipeList.size(); j++)
        {
            if (numpipeList[j].cnt <= 0)
            {
                isReadFromNumPipe = true;
                readFromNumPipeWrite = numpipeList[j].write;
                prevRead = numpipeList[j].read;
                numpipeList.erase(numpipeList.begin() + j);
                break;
            }
        }

        // check if the number pipe writing to already exists
        if (isNumPipe)
        {
            for (std::size_t j = 0; j < numpipeList.size(); j++)
            {
                if (numpipeList[j].cnt == cmds[i].cnt)
                {
                    isWriteToNumPipe = true;
                    writeToNumPipeWrite = numpipeList[j].write;
                    break;
                }
            }
        }

        // create pipe
        int pipedes[2] = {-1, -1};
        isPipe = (isOrdPipe && !isLast) || (isNumPipe && !isWriteToNumPipe) || isFile;
        if (isPipe)
        {
            while (pipe(pipedes) < 0)
                perror("[ERROR] pipe : ");
        }

        // fork
        while (true)
        {
            pid_t pid = fork();
            if (pid == -1) // error
                npWait(procList);
            else if (pid == 0) // child process
            {
                // dup prevRead to STDIN
                npDup(prevRead, STDIN_FD);
                // close the prevRead, note that if prevRead is same as STDIN, don't close
                if (prevRead != STDIN_FD)
                    npClose(prevRead);
                // if is reading from number pipe, close the write-end of that number pipe
                if (isReadFromNumPipe)
                    npClose(readFromNumPipeWrite);
                // if is file redirection
                if (isFile)
                {
                    int ffd;
                    while ((ffd = open((*(cmds[i].data.end() - 1)).c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0)
                        perror("[ERROR] open :");
                    npDup(ffd, STDOUT_FD);
                    npClose(ffd);
                    npExec(std::vector<std::string>(cmds[i].data.begin(), cmds[i].data.end() - 2));
                    break; // never reach here
                }
                // if new pipe created this round
                if (isPipe)
                {
                    npDup(pipedes[PIPE_WRITE], STDOUT_FD);
                    if (isNumPipe2)
                        npDup(pipedes[PIPE_WRITE], STDERR_FD);
                    npClose(pipedes[PIPE_READ]);
                    npClose(pipedes[PIPE_WRITE]);
                }
                else if (isWriteToNumPipe)
                {
                    npDup(writeToNumPipeWrite, STDOUT_FD);
                    if (isNumPipe2)
                        npDup(writeToNumPipeWrite, STDERR_FD);
                }

                //------------------------------------------------------------//
                // the remaining cases are output to stdout, so nothing to do //
                //------------------------------------------------------------//

                // execute command
                npExec(cmds[i].data);
                break; // never reach here
            }
            else // parnet process
            {
                // if is reading from number pipe, close the write-end of that number pipe
                if (isReadFromNumPipe)
                    npClose(readFromNumPipeWrite);
                // add new object to process list
                Proc p(pid, prevRead, STDOUT_FD);
                if (isPipe)
                    p.writeEnd = pipedes[PIPE_WRITE];
                else if (isWriteToNumPipe)
                    p.writeEnd = writeToNumPipeWrite;
                procList.push_back(p);
                // add new object to number pipe list
                if (isNumPipe && !isWriteToNumPipe) // condition : is number pipe + the number pipe writing to doesn't exist
                    numpipeList.push_back(Numpipe(pipedes[PIPE_READ], pipedes[PIPE_WRITE], cmds[i].cnt));
                // if is not number pipe, close unused pipe write-end
                if (isPipe && !isNumPipe)
                    npClose(pipedes[PIPE_WRITE]);
                // decrement number pipe list count
                if (!isLast && isNumPipe)
                {
                    for (auto &numpipe : numpipeList)
                        numpipe.cnt--;
                }

                // if is number pipe, reset the read-end to STDIN
                // otherwise, set to pip read-end
                prevRead = (!isPipe && isNumPipe) ? STDIN_FD : pipedes[PIPE_READ];
                break;
            }
        }
    }

    // deal with finishing child processes
    npWait(procList);
    return true;
}

void NpShell::npParse(std::vector<Cmd> &cmds, std::string str)
{
    cmds = std::vector<Cmd>({{{}, 0, '0'}});
    std::string delim = " ";
    std::string::size_type begin = 0;
    std::string::size_type end = str.find(delim);
    while (end != std::string::npos)
    {
        if (begin < end)
        {
            std::string temp = str.substr(begin, end - begin);
            if (*(temp.begin()) == '|' || *(temp.begin()) == '!')
            {
                if (temp.size() > 1)
                {
                    (*(cmds.end() - 1)).cnt = std::stol(std::string(temp.begin() + 1, temp.end()));
                    (*(cmds.end() - 1)).type = *(temp.begin());
                }
                cmds.push_back({{}, 0, '0'});
            }
            else
            {
                (*(cmds.end() - 1)).data.push_back(temp);
            }
        }
        begin = end + delim.size();
        end = str.find(delim, begin);
    }
    if (begin < str.size()) // deal with the last substring
    {
        std::string temp = str.substr(begin);
        if (*(temp.begin()) == '|' || *(temp.begin()) == '!')
        {
            if (temp.size() > 1)
            {
                (*(cmds.end() - 1)).cnt = stol(std::string(temp.begin() + 1, temp.end()));
                (*(cmds.end() - 1)).type = *(temp.begin());
            }
        }
        else
        {
            (*(cmds.end() - 1)).data.push_back(temp);
        }
    }
}

void NpShell::npSetenv(std::string var, std::string value)
{
    if (setenv(var.c_str(), value.c_str(), 1) < 0)
        perror("[ERROR] setenv : ");
}

void NpShell::npPrintenv(std::string var)
{
    char *env = getenv(var.c_str());
    if (env)
        std::cout << env << std::endl;
}

void NpShell::npExec(std::vector<std::string> cmds)
{
    char args[MAX_CMD_ARG][MAX_CMD_LEN];
    char *argp[MAX_CMD_ARG];
    for (std::size_t i = 0; i < cmds.size(); i++)
        std::strcpy(args[i], cmds[i].c_str());
    for (std::size_t i = 0; i < cmds.size(); i++)
        argp[i] = args[i];
    argp[cmds.size()] = (char *)0;

    if (execvp(argp[0], argp) < 0)
        std::cerr << "Unknown command: [" << argp[0] << "]." << std::endl;
    exit(1);
}

void NpShell::npExit()
{
    exit(0);
}

void NpShell::npDup(int fd1, int fd2)
{
    if (dup2(fd1, fd2) < 0)
        perror("[ERROR] npDup :");
    // else
    //     std::cout << "dup " << fd1 << " " << fd2 << std::endl;
}

void NpShell::npClose(int fd)
{
    if (close(fd) < 0)
        perror("[ERROR] npClose :");
    // else
    //     std::cout << "close " << fd << std::endl;
}

bool NpShell::npWait(std::vector<Proc> &procs)
{
    bool waitFlag = false;
    for (std::size_t i = 0; i < procs.size(); i++)
    {
        int opt = (procs[i].writeEnd == STDOUT_FD) ? 0 : WNOHANG; // only wait for processes outputing to stdout
        pid_t wpid = waitpid(procs[i].pid, (int *)0, opt);
        if (wpid > 0)
        {
            procs.erase(procs.begin() + i);
            waitFlag = true;
            i--;
        }
    }
    return waitFlag;
}

void NpShell::npWrite(int fd, std::string str)
{
    if (write(fd, str.c_str(), str.size()) < 0)
        perror("[ERROR] write :");
}

NpShell::NpShell()
{
    npSetenv("PATH", "bin:.");
}

NpShell::~NpShell()
{
    npWait(procList);
}