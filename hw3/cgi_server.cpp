#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <stdio.h>
#include <string.h>

// #include <boost/asio.hpp>

using boost::asio::ip::tcp;

#define MAX_LEN 15000

class Host
{
  public:
    std::string hostname;
    std::string port;
    std::string filename;
    std::string id;
    Host(std::string _hostname, std::string _port, std::string _filename, std::string _id) : hostname(_hostname), port(_port), filename(_filename), id(_id)
    {
    }
};

class Client : public std::enable_shared_from_this<Client>
{
  private:
    char rdata_[MAX_LEN];
    char wdata_[MAX_LEN];
    std::string sdata_;
    tcp::socket socket_;
    tcp::resolver resolver_;
    tcp::resolver::query query_;
    tcp::socket *ssocket_ptr_;

    Host host_;
    std::vector<std::string> fileline_;

  public:
    Client(boost::asio::io_context &io_context_, tcp::resolver::query query_, Host host_, tcp::socket *ssocket_ptr_)
        : socket_(io_context_), resolver_(io_context_), query_(move(query_)), host_(host_), ssocket_ptr_(ssocket_ptr_)
    {
    }

    void start()
    {
        fileline_ = readfile("test_case/" + host_.filename);
        do_resolve();
    }

    void do_resolve()
    {
        auto self(shared_from_this());
        resolver_.async_resolve(query_, [this, self](boost::system::error_code ec, tcp::resolver::iterator iter) {
            if (!ec)
            {
                auto self(shared_from_this());
                socket_.async_connect(*iter, [this, self](boost::system::error_code ec) {
                    if (!ec)
                    {
                        do_read();
                    }
                });
            }
        });
    }

    void do_read() // client reads from npshell
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(rdata_, sizeof(rdata_)), [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec)
            {
                std::string srdata_(rdata_);
                sdata_ += srdata_;
                std::memset(rdata_, 0, sizeof(rdata_));
                if (std::find(srdata_.begin(), srdata_.end(), '%') == srdata_.end()) // not found '%'
                {
                    do_read();
                }
                else // found '%'
                {
                    do_write_browser(html_insertion(host_.id, replace_html_escape(sdata_), false));
                    sdata_ = "";
                    string swdata_ = "";
                    if (!fileline_.empty())
                    {
                        std::strcpy(wdata_, ((*(fileline_.begin())) + "\r\n").c_str());
                        do_write_browser(html_insertion(host_.id, replace_html_escape((*(fileline_.begin())) + "\n"), true));
                        fileline_.erase(fileline_.begin());
                    }
                    do_write();
                }
            }
        });
    }

    void do_write() // client writes to npshell
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(wdata_, std::strlen(wdata_)), [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec)
            {
                std::memset(wdata_, 0, sizeof(wdata_));
                do_read();
            }
        });
    }

    void do_write_browser(string str) // client writes to browser
    {
        auto self(shared_from_this());

        // check no invisible character in the string
        for (std::size_t i = 0; i < str.size(); i++)
        {
            if (str[i] < 32)
            {
                str.erase(str.begin() + i);
                i--;
            }
        }

        boost::asio::async_write(*ssocket_ptr_, boost::asio::buffer(str.c_str(), str.size()), [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec)
            {
            }
        });
    }

    static std::vector<std::string> readfile(std::string filename)
    {
        std::fstream fs;
        fs.open(filename, ios::in);
        if (!fs.is_open())
        {
            std::cerr << "fs.open(): fail to open file." << std::endl;
            return {};
        }
        std::vector<std::string> fileline;
        std::string line;
        while (std::getline(fs, line))
        {
            for (std::size_t i = 0; i < line.size(); i++)
            {
                if (line[i] < 32)
                {
                    line.erase(line.begin() + i);
                    i--;
                }
            }
            fileline.push_back(line);
        }
        fs.close();
        return fileline;
    }

    static std::string html_insertion(std::string id, std::string content, bool is_cmd)
    {
        return std::string("<script>document.getElementById('") + id + "').innerHTML += '" + (is_cmd ? "<b>" : "") + content + (is_cmd ? "</b>" : "") + "';</script>\n";
    }

    static std::string replace_html_escape(std::string str)
    {
        std::string ret;
        for (std::size_t i = 0; i < str.size(); i++)
        {
            switch (str[i])
            {
            case '&':
                ret += "&amp;";
                break;
            case '\"':
                ret += "&quot;";
                break;
            case '\'':
                ret += "&apos;";
                break;
            case '<':
                ret += "&lt;";
                break;
            case '>':
                ret += "&gt;";
                break;
            case '\r':
                break;
            case '\n':
                ret += "<br>";
                break;
            default:
                if ((int)str[i] >= 32)
                    ret += str[i];
            }
        }
        return ret;
    }
};

class Session : public std::enable_shared_from_this<Session>
{
  private:
    tcp::socket socket_;
    char rdata_[MAX_LEN];
    char wdata_[4096];

    std::string request_method;
    std::string request_uri;
    std::string query_string;
    std::string server_protocol;
    std::string http_host;
    std::string server_addr;
    std::string server_port;
    std::string remote_addr;
    std::string remote_port;
    std::string cgi;
    pid_t pid;

    void print_request()
    {
        std::cout << cgi << std::endl;
    }

    void do_read() // server reads from browser
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(rdata_, MAX_LEN), [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec)
            {
                std::stringstream ss;
                std::string dummy;

                ss << std::string(rdata_);
                ss >> request_method >> request_uri >> server_protocol >> dummy >> http_host;
                server_addr = socket_.local_endpoint().address().to_string();
                server_port = std::to_string(socket_.local_endpoint().port());
                remote_addr = socket_.remote_endpoint().address().to_string();
                remote_port = std::to_string(socket_.remote_endpoint().port());

                auto it = std::find(request_uri.begin(), request_uri.end(), '?');
                cgi = (it == request_uri.end()) ? std::string(request_uri.begin() + 1, it) : request_uri;
                query_string = (it == request_uri.end()) ? std::string(it + 1, request_uri.end()) : "";

                print_request();

                if (cgi == "panel.cgi")
                    print_panel();
                else if (cgi == "console.cgi")
                    print_console();
            }
        });
    }

    void do_write(std::string str) // server writes to browser
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(str.c_str(), str.size()), [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec)
            {
            }
        });
    }

    void print_panel()
    {
        do_write("HTTP/1.1 200 OK\r\n");
        do_write("Content-Type: text/html\r\n\r\n");
        do_write(R"""(
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <title>NP Project 3 Panel</title>
                <link
                rel="stylesheet"
                href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
                integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
                crossorigin="anonymous"
                />
                <link
                href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
                rel="stylesheet"
                />
                <link
                rel="icon"
                type="image/png"
                href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png"
                />
                <style>
                * {
                    font-family: 'Source Code Pro', monospace;
                }
                </style>
            </head>
            <body class="bg-secondary pt-5">
                <form action="console.cgi" method="GET">
                <table class="table mx-auto bg-light" style="width: inherit">
                    <thead class="thead-dark">
                    <tr>
                        <th scope="col">#</th>
                        <th scope="col">Host</th>
                        <th scope="col">Port</th>
                        <th scope="col">Input File</th>
                    </tr>
                    </thead>
                    <tbody>
                    <tr>
                        <th scope="row" class="align-middle">Session 1</th>
                        <td>
                        <div class="input-group">
                            <select name="h0" class="custom-select">
                            <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                            </select>
                            <div class="input-group-append">
                            <span class="input-group-text">.cs.nctu.edu.tw</span>
                            </div>
                        </div>
                        </td>
                        <td>
                        <input name="p0" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                        <select name="f0" class="custom-select">
                            <option></option>
                            <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                        </select>
                        </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 2</th>
                        <td>
                        <div class="input-group">
                            <select name="h1" class="custom-select">
                            <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                            </select>
                            <div class="input-group-append">
                            <span class="input-group-text">.cs.nctu.edu.tw</span>
                            </div>
                        </div>
                        </td>
                        <td>
                        <input name="p1" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                        <select name="f1" class="custom-select">
                            <option></option>
                            <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                        </select>
                        </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 3</th>
                        <td>
                        <div class="input-group">
                            <select name="h2" class="custom-select">
                            <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                            </select>
                            <div class="input-group-append">
                            <span class="input-group-text">.cs.nctu.edu.tw</span>
                            </div>
                        </div>
                        </td>
                        <td>
                        <input name="p2" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                        <select name="f2" class="custom-select">
                            <option></option>
                            <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                        </select>
                        </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 4</th>
                        <td>
                        <div class="input-group">
                            <select name="h3" class="custom-select">
                            <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                            </select>
                            <div class="input-group-append">
                            <span class="input-group-text">.cs.nctu.edu.tw</span>
                            </div>
                        </div>
                        </td>
                        <td>
                        <input name="p3" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                        <select name="f3" class="custom-select">
                            <option></option>
                            <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                        </select>
                        </td>
                    </tr>
                    <tr>
                        <th scope="row" class="align-middle">Session 5</th>
                        <td>
                        <div class="input-group">
                            <select name="h4" class="custom-select">
                            <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                            </select>
                            <div class="input-group-append">
                            <span class="input-group-text">.cs.nctu.edu.tw</span>
                            </div>
                        </div>
                        </td>
                        <td>
                        <input name="p4" type="text" class="form-control" size="5" />
                        </td>
                        <td>
                        <select name="f4" class="custom-select">
                            <option></option>
                            <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                        </select>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="3"></td>
                        <td>
                        <button type="submit" class="btn btn-info btn-block">Run</button>
                        </td>
                    </tr>
                    </tbody>
                </table>
                </form>
            </body>
            </html>
        )""");
    }

    std::vector<Host> parse(std::string query_string)
    {
        std::vector<std::string> temp = {""};
        for (std::size_t i = 0; i < query_string.size(); i++)
        {
            if (query_string[i] == '&')
                temp.push_back("");
            else
                temp[temp.size() - 1] += query_string[i];
        }
        std::vector<Host> host_list;
        for (std::size_t i = 0; i < temp.size(); i += 3)
        {
            if (temp[i].size() > 3 && temp[i + 1].size() > 3 && temp[i + 2].size() > 3)
            {
                host_list.push_back(Host(std::string(temp[i].begin() + 3, temp[i].end()),         // hostname
                                         std::string(temp[i + 1].begin() + 3, temp[i + 1].end()), // port
                                         std::string(temp[i + 2].begin() + 3, temp[i + 2].end()), // filename
                                         "s" + std::to_string(i))                                 // id
                );
            }
        }
        return host_list;
    }

    void print_console()
    {
        std::vector<Host> host_list = parse(query_string);
        do_write("HTTP/1.1 200 OK\r\n");
        do_write("Content-type: text/html\r\n\r\n");
        do_write(R"""(
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8" />
                <title>NP Project 3 Console</title>
                <link
                rel="stylesheet"
                href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
                integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
                crossorigin="anonymous"
                />
                <link
                href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
                rel="stylesheet"
                />
                <link
                rel="icon"
                type="image/png"
                href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
                />
                <style>
                * {
                    font-family: 'Source Code Pro', monospace;
                    font-size: 1rem !important;
                }
                body {
                    background-color: #212529;
                }
                pre {
                    color: #cccccc;
                }
                b {
                color: #01b468;
                }
                </style>
                </head>
                <body>
                    <table class="table table-dark table-bordered">
                    <thead>
                        <tr id="s_title">
                        </tr>
                    </thead>
                    <tbody>
                        <tr id="s_body">
                        </tr>
                    </tbody>
                    </table>
                </body>
            </html>
        )""");
        for (std::size_t i = 0; i < host_list.size(); i++)
        {
            do_write(Client::html_insertion("s_title", "<th scope=\"col\">" + host_list[i].hostname + ":" + host_list[i].port + "</th>", false));
            do_write(Client::html_insertion("s_body", "<td><pre id=\"" + host_list[i].id + "\" class=\"mb-0\"></pre></td>", false));
        }

        boost::asio::io_context io_context;
        for (std::size_t i = 0; i < host_list.size(); i++)
        {
            tcp::resolver::query query(host_list[i].hostname, host_list[i].port);
            make_shared<Client>(io_context, move(query), host_list[i], &socket_)->start();
        }

        io_context.run();
    }

  public:
    Session(tcp::socket socket) : socket_(std::move(socket))
    {
    }

    void start()
    {
        do_read();
    }
};

class server
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
    server(boost::asio::io_context &io_context, short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
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
        server s(io_context, std::atoi(argv[1]));
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}