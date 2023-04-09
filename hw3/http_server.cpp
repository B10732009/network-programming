// c
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// c++
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class session: public std::enable_shared_from_this <session> {
    private: tcp::socket socket_;
    enum {max_length = 1024};
    char data_[max_length];

    string request_method;
    string request_uri;
    string query_string;
    string server_protocol;
    string http_host;
    string server_addr;
    string server_port;
    string remote_addr;
    string remote_port;
    string cgi;
    pid_t pid;

    void print_request() {
        cout << cgi << endl;
    }

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    stringstream ss;
                    ss << string(data_);
                    string dummy;
                    ss >> request_method >> request_uri >> server_protocol >> dummy >> http_host;
                    int loc;
                    for (loc = 0; loc < (int) request_uri.size(); loc++) {
                        if (request_uri[loc] == '?') {
                            break;
                        }
                    }
                    cgi = (loc < (int) request_uri.size()) ? string(request_uri.begin() + 1, request_uri.begin() + loc) : request_uri;
                    query_string = (loc < (int) request_uri.size()) ? string(request_uri.begin() + loc + 1, request_uri.end()) : "";
                    server_addr = socket_.local_endpoint().address().to_string();
                    server_port = to_string(socket_.local_endpoint().port());
                    remote_addr = socket_.remote_endpoint().address().to_string();
                    remote_port = to_string(socket_.remote_endpoint().port());
                    print_request();

                    pid = fork();
                    if (pid < 0) { // error
                        perror("fork()");
                        exit(1);
                    } 
                    else if (pid == 0) { // child
                        setenv("REQUEST_METHOD", request_method.c_str(), 1);
                        setenv("REQUEST_URI", request_uri.c_str(), 1);
                        setenv("QUERY_STRING", query_string.c_str(), 1);
                        setenv("SERVER_PROTOCOL", server_protocol.c_str(), 1);
                        setenv("HTTP_HOST", http_host.c_str(), 1);
                        setenv("SERVER_ADDR", server_addr.c_str(), 1);
                        setenv("SERVER_PORT", server_port.c_str(), 1);
                        setenv("REMOTE_ADDR", remote_addr.c_str(), 1);
                        setenv("REMOTE_PORT", remote_port.c_str(), 1);

                        cout << cgi << endl;
                        if (dup2(socket_.native_handle(), STDIN_FILENO) < 0)
                            perror("dup2(STDIN_FILENO)");
                        if (dup2(socket_.native_handle(), STDOUT_FILENO) < 0)
                            perror("dup2(STDOUT_FILENO)");
                        socket_.close();

                        if (cgi != "/panel.cgi") {
                            cout << "HTTP/1.1 403 Forbidden\r\n";
                            //cout<<"block";
                            exit(0);
                        }

                        cout << "HTTP/1.1 200 OK\r\n";
                        cout << "Content-Type: text/html\r\n";
                        fflush(stdout);

                        char args[256];
                        strcpy(args, string("./" + cgi).c_str());
                        char * argp[2];
                        argp[0] = args;
                        argp[1] = NULL;
                        if (execvp(argp[0], argp) < 0)
                            perror("execvp()");
                        exit(0);
                    } 
                    else { // parent
                        socket_.close();
                        /*if(waitpid(pid, NULL, 0)<0){
                            perror("waitpid()");
                            exit(1);
                        }*/
                    }
                    do_read();
                    //do_write(length);
                }
            }
        );
    }

    /*void do_write(std::size_t length){
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
            [this, self](boost::system::error_code ec, std::size_t ){
                if (!ec){
                    do_read();
                }
            }
        );
    }*/

    public: session(tcp::socket socket): socket_(std::move(socket)) {}

    void start() {
        do_read();
    }
};

class server {
    private: tcp::acceptor acceptor_;

    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared < session > (std::move(socket)) -> start();
                }
                do_accept();
            });
    }

    public: server(boost::asio::io_context & io_context, short port): acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }
};

int main(int argc, char * argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        server s(io_context, std::atoi(argv[1]));
        io_context.run();
    } 
    catch (std::exception & e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}