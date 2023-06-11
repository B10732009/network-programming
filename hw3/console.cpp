#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#define MAX_LEN 15000

class Host
{
  public:
    string hostname;
    string port;
    string filename;
    string id;
    Host(string _hostname, string _port, string _filename, string _id) : hostname(_hostname), port(_port), filename(_filename), id(_id)
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

    Host host_;
    std::vector<std::string> fileline_;

  public:
    Client(boost::asio::io_context &io_context_, tcp::resolver::query query_, Host host_) : socket_(io_context_), resolver_(io_context_), query_(move(query_)), host_(host_)
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

    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(rdata_, sizeof(rdata_)), [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec)
            {
                std::string srdata_(rdata_);
                sdata_ += srdata_;
                std::memset(rdata_, 0, sizeof(rdata_));
                if (find(srdata_.begin(), srdata_.end(), '%') == srdata_.end()) // not found '%'
                {
                    do_read();
                }
                else // found '%'
                {
                    print_html_insertion(host_.id, replace_html_escape(sdata_), false);
                    sdata_ = "";
                    string swdata_ = "";
                    if (!fileline_.empty())
                    {
                        std::strcpy(wdata_, ((*(fileline_.begin())) + "\r\n").c_str());
                        print_html_insertion(host_.id, replace_html_escape((*(fileline_.begin())) + "\n"), true); // cout<<(*(fileline_.begin()))+;
                        fflush(stdout);
                        fileline_.erase(fileline_.begin());
                    }
                    do_write();
                }
            }
        });
    }

    void do_write()
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(wdata_, strlen(wdata_)), [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec)
            {
                std::memset(wdata_, 0, sizeof(wdata_));
                do_read();
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

    static void print_html_insertion(std::string id, std::string content, bool is_cmd)
    {
        for (std::size_t i = 0; i < content.size(); i++)
        {
            if (content[i] < 32)
            {
                content.erase(content.begin() + i);
                i--;
            }
        }
        std::cout << "<script>document.getElementById('" << id << "').innerHTML += '" << (is_cmd ? "<b>" : "") << content << (is_cmd ? "</b>" : "") << "';</script>" << std::endl;
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
                if (str[i] >= 32)
                    ret += str[i];
            }
        }
        return ret;
    }
};

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

int main()
{
    std::string query_string = getenv("QUERY_STRING");
    std::vector<Host> host_list = parse(query_string);

    std::cout << "Content-type: text/html\r\n\r\n" << std::flush << std::endl;
    string html_base = R"""(
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
    )""";

    for (std::size_t i = 0; i < html_base.size(); i++)
    {
        if (html_base[i] < 32)
        {
            html_base.erase(html_base.begin() + i);
            i--;
        }
    }

    std::cout << html_base << std::flush << std::endl;
    for (std::size_t i = 0; i < host_list.size(); i++)
    {
        Client::print_html_insertion("s_title", "<th scope=\"col\">" + host_list[i].hostname + ":" + host_list[i].port + "</th>", false);
        Client::print_html_insertion("s_body", "<td><pre id=\"" + host_list[i].id + "\" class=\"mb-0\"></pre></td>", false);
    }
    std::fflush(stdout);

    boost::asio::io_context io_context;
    for (std::size_t i = 0; i < host_list.size(); i++)
    {
        tcp::resolver::query query(host_list[i].hostname, host_list[i].port);
        make_shared<Client>(io_context, move(query), host_list[i])->start();
    }

    io_context.run();
    return 0;
}
