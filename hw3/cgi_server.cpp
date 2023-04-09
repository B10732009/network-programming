// c
#include <stdio.h>
#include <string.h>

// c++
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class Host{
public:
    string hostname;
    string port;
    string filename;
    string id;
    Host(string _hostname, string _port, string _filename, string _id):
        hostname(_hostname), port(_port), filename(_filename), id(_id){
    }
};

class Client: public std::enable_shared_from_this<Client>{
private:
    enum { max_length = 15000 };
    char rdata_[max_length];
    char wdata_[max_length];
    string sdata_;
    tcp::socket socket_;
    tcp::resolver resolver_;
    tcp::resolver::query query_;
    Host host_;
    tcp::socket* ssocket_ptr_;
    vector<string> fileline_;

public:
    Client(boost::asio::io_context& io_context_, tcp::resolver::query query_, Host host_, tcp::socket* ssocket_ptr_):
            socket_(io_context_), resolver_(io_context_), query_(move(query_)), host_(host_), ssocket_ptr_(ssocket_ptr_){
    }

    void start(){
        fileline_ = readfile("test_case/"+host_.filename);
        do_resolve();
    }

    void do_resolve(){
        auto self(shared_from_this());
        resolver_.async_resolve(query_, 
            [this, self](boost::system::error_code ec, tcp::resolver::iterator iter){
                if(!ec){
                    auto self(shared_from_this());
                    socket_.async_connect(*iter,
                        [this, self](boost::system::error_code ec){
                            if(!ec){
                                do_read();
                            }
                        }
                    );
                }
            }
        );
    }

    void do_read(){ // client reads from npshell 
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(rdata_, sizeof(rdata_)),
            [this, self](boost::system::error_code ec, std::size_t length){
                if (!ec){
                    string srdata_(rdata_);
                    sdata_ += srdata_;
                    memset(rdata_, 0, sizeof(rdata_));
                    if(find(srdata_.begin(), srdata_.end(), '%')==srdata_.end()){ // not found '%'
                        do_read();
                    }
                    else{ // found '%'
                        do_write_browser(html_insertion(host_.id, replace_html_escape(sdata_), false));
                        fflush(stdout);
                        sdata_ = "";

                        string swdata_ = "";
                        if(!fileline_.empty()){
                            strcpy(wdata_, ((*(fileline_.begin()))+"\r\n").c_str());
                            do_write_browser(html_insertion(host_.id, replace_html_escape((*(fileline_.begin()))+"\n"), true));
                            fflush(stdout);
                            fileline_.erase(fileline_.begin());
                        }
                        do_write();
                    }
                }
            }
        );
    }

    void do_write(){ // client writes to npshell
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(wdata_, strlen(wdata_)),
            [this, self](boost::system::error_code ec, std::size_t){
                if (!ec){
                    memset(wdata_, 0, sizeof(wdata_));
                    do_read();
                }
            }
        );
    }

    void do_write_browser(string str){ // client writes to browser
        auto self(shared_from_this());
        for(int i = 0; i < (int)str.size(); i++){ // to check no invisible character in the string
            if((int)str[i] < 32){
                str.erase(str.begin()+i);
                i--;
            }
        }
        boost::asio::async_write(*ssocket_ptr_, boost::asio::buffer(str.c_str(), str.size()),
            [this, self](boost::system::error_code ec, std::size_t){
                if (!ec){
                }
            }
        );
    }

    static vector<string> readfile(string filename){
        fstream fs;
        fs.open(filename, ios::in);
        if(!fs.is_open()){
            cerr<<"fs.open(): fail to open file."<<endl;
            return {};
        }
        vector<string> fileline;
        string line;
        while(getline(fs, line)){
            for(int i = 0; i < (int)line.size(); i++){
                if(line[i] < 32){
                    line.erase(line.begin()+i);
                    i--;
                }
            }
            fileline.push_back(line);
        }
        fs.close();
        return fileline;
    }

    static string html_insertion(string id, string content, bool is_cmd){
        return string("<script>document.getElementById('") + id + "').innerHTML += '" +
            (is_cmd ? "<b>" : "") + content + (is_cmd ? "</b>" : "") + "';</script>\n";
    }

    static string replace_html_escape(string str){
        string ret;
        for(int i = 0; i < (int)str.size(); i++){
            switch(str[i]){
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
                    if((int)str[i] >= 32){
                        ret += str[i];
                    }  
            }
        }
        return ret;
    }
};

class session: public std::enable_shared_from_this<session>{
private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char rdata_[max_length];
    char wdata_[4096];

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

    void print_request(){
        cout << cgi << endl;
    }

    void do_read(){ // server reads from browser
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(rdata_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length){
                if (!ec){
                    stringstream ss;
                    ss << string(rdata_);
                    string dummy;
                    ss >> request_method >> request_uri >> server_protocol >> dummy >> http_host;
                    int loc;
                    for(loc = 0; loc < (int)request_uri.size(); loc++){
                        if(request_uri[loc] == '?'){
                            break;
                        }
                    }
                    cgi = (loc < (int)request_uri.size()) ? string(request_uri.begin()+1, request_uri.begin()+loc) : string(request_uri.begin()+1, request_uri.end());
                    query_string = (loc < (int)request_uri.size()) ? string(request_uri.begin()+loc+1, request_uri.end()) : "";
                    server_addr = socket_.local_endpoint().address().to_string();
                    server_port = to_string(socket_.local_endpoint().port());
                    remote_addr = socket_.remote_endpoint().address().to_string();
                    remote_port = to_string(socket_.remote_endpoint().port());
                    print_request();

                    if(cgi == "panel.cgi"){
                        print_panel();
                    }
                    else if(cgi == "console.cgi"){
                        print_console();
                    }
                }
            }
        );
    }

    void do_write(string str){ // server writes to browser
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(str.c_str(), str.size()),
            [this, self](boost::system::error_code ec, std::size_t){
                if (!ec){
                }
            }
        );
    }

    void print_panel(){
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

    vector<Host> parse(string query_string){
        vector<string> temp = {""};
        for(int i = 0; i < (int)query_string.size(); i++){
            if(query_string[i] == '&')
                temp.push_back("");
            else
                temp[temp.size()-1] += query_string[i];
        }
        vector<Host> host_list;
        for(int i = 0; i < (int)temp.size(); i += 3){
            if((int)temp[i].size() > 3 && (int)temp[i+1].size() > 3 && (int)temp[i+2].size() > 3){
                host_list.push_back(Host(
                    string(temp[i].begin()+3, temp[i].end()),
                    string(temp[i+1].begin()+3, temp[i+1].end()),
                    string(temp[i+2].begin()+3, temp[i+2].end()),
                    "s" + to_string(i)
                ));
            }
        }
        return host_list;
    }

    void print_console(){
        vector<Host> host_list = parse(query_string);
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
        for(int i = 0; i < (int)host_list.size(); i++){
            do_write(Client::html_insertion("s_title", "<th scope=\"col\">" + host_list[i].hostname + ":" + host_list[i].port + "</th>", false));
            do_write(Client::html_insertion("s_body", "<td><pre id=\"" + host_list[i].id + "\" class=\"mb-0\"></pre></td>", false));
        }

        boost::asio::io_context io_context;
        for(int i = 0; i < (int)host_list.size(); i++){
            tcp::resolver::query query(host_list[i].hostname, host_list[i].port);
            make_shared<Client>(io_context, move(query), host_list[i], &socket_)->start();
        }

        io_context.run();
    }
    
public:
    session(tcp::socket socket): socket_(std::move(socket)){
    }

    void start(){
        do_read();
    }
};

class server{
private:
    tcp::acceptor acceptor_;

    void do_accept(){
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket){
            if (!ec){
                std::make_shared<session>(std::move(socket))->start();
            }
            do_accept();
            });
    }
public:
    server(boost::asio::io_context& io_context, short port): acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
        do_accept();
    }
};

int main(int argc, char* argv[])
{
    try{
        if (argc != 2){
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        server s(io_context, std::atoi(argv[1]));
        io_context.run();
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}