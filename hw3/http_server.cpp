#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#define MAX_LEN 1024

class Session : public std::enable_shared_from_this<Session>
{
  private:
    // socket for connection
    tcp::socket socket_;

    // data buffer
    char data_[MAX_LEN];

    // environment variable
    std::string request_method;
    std::string request_uri;
    std::string query_string;
    std::string server_protocol;
    std::string http_host;
    std::string server_addr;
    std::string server_port;
    std::string remote_addr;
    std::string remote_port;

    // cgi string
    std::string cgi;

    // pid for fork()
    pid_t pid;

    void print_request()
    {
        std::cout << cgi << std::endl;
    }

    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, MAX_LEN), [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec)
            {
                std::stringstream ss;
                std::string dummy;

                ss << std::string(data_);
                ss >> request_method >> request_uri >> server_protocol >> dummy >> http_host;
                server_addr = socket_.local_endpoint().address().to_string();
                server_port = std::to_string(socket_.local_endpoint().port());
                remote_addr = socket_.remote_endpoint().address().to_string();
                remote_port = std::to_string(socket_.remote_endpoint().port());

                auto it = std::find(request_uri.begin(), request_uri.end(), '?');
                cgi = (it == request_uri.end()) ? std::string(request_uri.begin() + 1, it) : request_uri;
                query_string = (it == request_uri.end()) ? std::string(it + 1, request_uri.end()) : "";

                print_request();

                pid = fork();
                if (pid < 0) // error
                {
                    perror("[ERROR] fork :");
                    exit(1);
                }
                else if (pid == 0) // child
                {
                    // set environment variables
                    setenv("REQUEST_METHOD", request_method.c_str(), 1);
                    setenv("REQUEST_URI", request_uri.c_str(), 1);
                    setenv("QUERY_STRING", query_string.c_str(), 1);
                    setenv("SERVER_PROTOCOL", server_protocol.c_str(), 1);
                    setenv("HTTP_HOST", http_host.c_str(), 1);
                    setenv("SERVER_ADDR", server_addr.c_str(), 1);
                    setenv("SERVER_PORT", server_port.c_str(), 1);
                    setenv("REMOTE_ADDR", remote_addr.c_str(), 1);
                    setenv("REMOTE_PORT", remote_port.c_str(), 1);

                    // dup socket to stdin, stdout
                    if (dup2(socket_.native_handle(), STDIN_FILENO) < 0)
                        perror("dup2(STDIN_FILENO)");
                    if (dup2(socket_.native_handle(), STDOUT_FILENO) < 0)
                        perror("dup2(STDOUT_FILENO)");
                    // close socket (already dupped to stdin, stdout)
                    socket_.close();

                    // // set to only let panel.cgi come in
                    // if (cgi != "/panel.cgi")
                    // {
                    //     std::cerr << "HTTP/1.1 403 Forbidden\r\n";
                    //     exit(0);
                    // }

                    // output HTML header
                    std::cout << "HTTP/1.1 200 OK\r\n" << std::flush;
                    std::cout << "Content-Type: text/html\r\n" << std::flush;

                    // execute cgi
                    char args[256];
                    strcpy(args, std::string("./" + cgi).c_str());
                    char *argp[2] = {args, (char *)0};
                    if (execvp(argp[0], argp) < 0)
                        perror("[ERROR] execvp :");
                    exit(1); // should never reach
                }
                else // parent
                {
                    // close socket (already dupped to stdin, stdout in child process)
                    socket_.close();

                    // wait child process
                    // if (waitpid(pid, NULL, 0) < 0)
                    // {
                    //     perror("waitpid()");
                    //     exit(1);
                    // }

                    // read the following data
                    do_read();
                }
            }
        });
    }

    // void do_write(std::size_t length){
    //     auto self(shared_from_this());
    //     boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
    //         [this, self](boost::system::error_code ec, std::size_t ){
    //             if (!ec){
    //                 do_read();
    //             }
    //         }
    //     );
    // }

  public:
    Session(tcp::socket socket) : socket_(std::move(socket))
    {
    }

    void start()
    {
        do_read();
    }
};

class Server
{
  private:
    tcp::acceptor acceptor_;

    void do_accept()
    {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec)
            {
                std::make_shared<Session>(std::move(socket))->start();
            }
            do_accept();
        });
    }

  public:
    Server(boost::asio::io_context &io_context, short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
    }
};

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        Server s(io_context, std::atoi(argv[1]));
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}