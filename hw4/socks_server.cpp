#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class Socks4Request
{
  public:
    unsigned char vn;
    unsigned char cd;
    std::string port;
    std::string ip;

    Socks4Request()
    {
    }

    void parse(const unsigned char *buf, tcp::resolver *resolver)
    {
        vn = buf[0];
        cd = buf[1];
        port = std::to_string((((int)buf[2]) << 8) + (int)buf[3]);

        if (buf[4] == 0 && buf[5] == 0 && buf[6] == 0 && buf[7] != 0) // socks4a format 0.0.0.x
        {
            std::string domain = "";
            int i = 8;
            while (buf[i] != 0) // skip userid
                i++;
            i++;
            while (buf[i] != 0) // domain name
            {
                domain += buf[i];
                i++;
            }
            // resolve domain name to ip
            tcp::resolver::query query(domain, port);
            tcp::resolver::iterator iter = resolver->resolve(query);
            tcp::endpoint endpoint = *iter;
            ip = endpoint.address().to_string();
        }
        else // normal socks4 format
        {
            ip = std::to_string((int)buf[4]) + "." + std::to_string((int)buf[5]) + "." + std::to_string((int)buf[6]) + "." + std::to_string((int)buf[7]);
        }
    }
};

class Server
{
  private:
    boost::asio::io_context &io_context_;
    tcp::acceptor acceptor_;
    tcp::acceptor bacceptor_; // bind acceptor
    tcp::resolver resolver_;  // resolver
    tcp::socket ssock;        // src socket
    tcp::socket dsock;        // dest socket
    Socks4Request s4req;      // parsed socks4 request
    // std::vector<std::string> black_list;
    // bool is_in_black_list = false;

    unsigned char rbuf[15000]; // request buffer
    unsigned char sbuf[15000]; // source buffer
    unsigned char dbuf[15000]; // destination buffer
    unsigned char crep[8];     // connect reply
    unsigned char brep[8];     // bind reply

    // void check_black_list()
    // {
    //     for (int i = 0; i < black_list.size(); i++)
    //         cout << black_list[i] << endl;

    //     // cout<<"here"<<endl;

    //     int cnt = 0;
    //     for (int i = 0; i < (int)black_list.size(); i++)
    //     {
    //         if (black_list[i] == s4req.ip)
    //         {
    //             cnt++;
    //         }
    //     }

    //     if (cnt >= 3)
    //     {
    //         is_in_black_list = true;
    //     }
    //     else
    //     {
    //         is_in_black_list = false;
    //         black_list.push_back(s4req.ip);
    //     }
    // }

    void do_accept()
    {
        acceptor_.async_accept(ssock, [this](boost::system::error_code ec) {
            if (!ec)
            {
                do_read_request();
            }
        });
    }

    void do_read_request()
    {
        std::memset(rbuf, 0, sizeof(rbuf));
        ssock.async_read_some(boost::asio::buffer(rbuf, sizeof(rbuf)), [this](boost::system::error_code ec, size_t length) {
            if (!ec)
            {
                s4req.parse(rbuf, &resolver_);
                // check_black_list();

                io_context_.notify_fork(boost::asio::io_context::fork_prepare);
                pid_t pid = fork();
                if (pid < 0) // fork error
                {
                    perror("fork()");
                }
                else if (pid == 0) // child
                {
                    io_context_.notify_fork(boost::asio::io_context::fork_child);
                    do_reply();
                }
                else // parent
                {
                    io_context_.notify_fork(boost::asio::io_context::fork_parent);
                    ssock.close();
                    dsock.close();
                    do_accept();
                }
            }
        });
    }

    void do_reply()
    {
        std::string command = (s4req.cd == (unsigned char)1) ? "CONNECT" : "BIND";
        std::string reply = (s4req.vn == (unsigned char)4 && check_firewall(((command == "CONNECT") ? "c" : "b"), s4req.ip)) ? "Accept" : "Reject";
        do_print_msg(command, reply);
        if (reply == "Accept") // accept
        {
            if (command == "CONNECT") // connect
            {
                unsigned char rep[8] = {0, 90, 0, 0, 0, 0, 0, 0};
                boost::asio::async_write(ssock, boost::asio::buffer(rep, 8), [this](boost::system::error_code ec, size_t length) {
                    if (!ec)
                    {
                        tcp::endpoint ep(boost::asio::ip::address::from_string(s4req.ip), std::stoi(s4req.port));
                        dsock.connect(ep);
                        do_read_from_src();
                        do_read_from_dest();
                    }
                });
            }
            else // bind
            {
                tcp::endpoint endpoint(boost::asio::ip::address::from_string("0.0.0.0"), 0);
                bacceptor_.open(tcp::v4());
                bacceptor_.set_option(tcp::acceptor::reuse_address(true));
                bacceptor_.bind(endpoint);
                bacceptor_.listen(boost::asio::socket_base::max_connections);
                int aport = bacceptor_.local_endpoint().port();

                unsigned char rep[8] = {0, 90, (unsigned char)((aport >> 8) & 0xff), (unsigned char)(aport & 0xff), 0, 0, 0, 0};
                std::memcpy(brep, rep, 8);
                boost::asio::async_write(ssock, boost::asio::buffer(brep, 8), [this](boost::system::error_code ec, size_t length) {
                    if (!ec)
                    {
                        bacceptor_.accept(dsock);
                        boost::asio::async_write(ssock, boost::asio::buffer(brep, 8), [this](boost::system::error_code ec, size_t length) {
                            if (!ec)
                            {
                                do_read_from_src();
                                do_read_from_dest();
                            }
                        });
                    }
                });
            }
        }
        else // reject
        {
            unsigned char rep[8] = {0, 91, 0, 0, 0, 0, 0, 0};
            boost::asio::async_write(ssock, boost::asio::buffer(rep, 8), [this](boost::system::error_code ec, size_t length) {
                if (!ec)
                {
                }
            });
        }
    }

    void do_print_msg(std::string command, std::string reply)
    {
        std::cout << std::endl;
        std::cout << "<S_IP>: " << ssock.remote_endpoint().address().to_string() << std::endl;
        std::cout << "<S_PORT>: " << std::to_string(ssock.remote_endpoint().port()) << std::endl;
        std::cout << "<D_IP>: " << s4req.ip << std::endl;
        std::cout << "<D_PORT>: " << s4req.port << std::endl;
        std::cout << "<Command>: " << command << std::endl;
        std::cout << "<Reply>: " << reply << std::endl;
        std::cout << std::endl;
        std::fflush(stdout);
    }

    bool check_firewall(std::string type, std::string ip)
    {
        // open .conf file
        std::fstream fs;
        fs.open("socks.conf", std::ios::in);
        if (!fs.is_open())
        {
            std::cerr << "fs.open(): fail to open file." << std::endl;
            return false;
        }
        // check firewall
        std::string fw_permit, fw_type, fw_ip;
        while (fs >> fw_permit >> fw_type >> fw_ip)
        {
            // check type
            if (fw_type != type)
                continue;
            // parse ip and firewall ip
            int now = 0;
            std::vector<int> iip(4, 0);
            for (std::size_t i = 0; i < ip.size(); i++)
            {
                if (ip[i] == '.')
                    now++;
                else
                    iip[now] = iip[now] * 10 + (int)(ip[i] - '0');
            }
            // parse firewall ip
            now = 0;
            std::vector<int> fw_iip(4, 0);
            for (std::size_t i = 0; i < fw_ip.size(); i++)
            {
                if (fw_ip[i] == '.')
                    now++;
                else if (fw_ip[i] == '*')
                    fw_iip[now] = -1;
                else
                    fw_iip[now] = fw_iip[now] * 10 + (int)(fw_ip[i] - '0');
            }
            // check ip
            if ((fw_iip[0] == -1 || fw_iip[0] == iip[0]) && (fw_iip[1] == -1 || fw_iip[1] == iip[1]) && (fw_iip[2] == -1 || fw_iip[2] == iip[2]) && (fw_iip[3] == -1 || fw_iip[3] == iip[3]))
            {
                fs.close();
                return true;
            }
        }
        fs.close();
        return false;
    }

    void do_read_from_src()
    {
        std::memset(sbuf, 0, sizeof(sbuf));
        ssock.async_read_some(boost::asio::buffer(sbuf, sizeof(sbuf)), [this](boost::system::error_code ec, size_t length) {
            if (!ec)
            {
                do_write_to_dest(length);
            }
            else if (ec == boost::asio::error::eof)
            {
                ssock.close();
                dsock.close();
            }
        });
    }

    void do_write_to_dest(size_t length)
    {
        boost::asio::async_write(dsock, boost::asio::buffer(sbuf, length), [this](boost::system::error_code ec, size_t length) {
            if (!ec)
            {
                do_read_from_src();
            }
        });
    }

    void do_read_from_dest()
    {
        std::memset(dbuf, 0, sizeof(dbuf));
        dsock.async_read_some(boost::asio::buffer(dbuf, sizeof(dbuf)), [this](boost::system::error_code ec, size_t length) {
            if (!ec)
            {
                do_write_to_src(length);
            }
            else if (ec == boost::asio::error::eof)
            {
                ssock.close();
                dsock.close();
            }
        });
    }

    void do_write_to_src(size_t length)
    {
        boost::asio::async_write(ssock, boost::asio::buffer(dbuf, length), [this](boost::system::error_code ec, size_t) {
            if (!ec)
            {
                do_read_from_dest();
            }
        });
    }

  public:
    Server(boost::asio::io_context &io_context, short port)
        : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), bacceptor_(io_context), resolver_(io_context), ssock(io_context), dsock(io_context)
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
            std::cerr << "Usage: ./socks_server <port>" << std::endl;
            return 1;
        }

        boost::asio::io_context io_context;
        Server server(io_context, atoi(argv[1]));
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}

// curl --socks4a localhost:12000 www.google.com