#include <iostream>
#include <vector>
#include <string>
#include <fstream>
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

    Host(){
    }

    Host(string _hostname, string _port, string _filename, string _id):
        hostname(_hostname), port(_port), filename(_filename), id(_id){
    }
};

class SocksHost{
public:
    string hostname;
    string port;

    SocksHost(){
    }

    SocksHost(string _hostname, string _port):
        hostname(_hostname), port(_port){
    }
};

class Client: public std::enable_shared_from_this<Client>{
private:
    char rdata_[15000];
    char wdata_[15000];
    char s4data_[8];
    string sdata_;
    tcp::socket socket_;
    tcp::resolver resolver_;
    tcp::resolver::query query_;
    Host host_;
    SocksHost sockshost_;
    vector<string> fileline_;
    
public:
    Client(boost::asio::io_context& io_context_, tcp::resolver::query query_, Host host_, SocksHost sockshost_):
            socket_(io_context_), resolver_(io_context_), query_(move(query_)), host_(host_), sockshost_(sockshost_){
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
                                send_s4_request();
                            }
                        }
                    );
                }
            }
        );
    }

    void send_s4_request(){
        // construct a socks4a string
        string s4a = "";
        s4a += (char)4; // VN
        s4a += (char)1; // CD
        s4a += (char)(stoi(host_.port)/256); // DSTPORT high
        s4a += (char)(stoi(host_.port)%256); // DSTPORT low
        s4a += (char)0; // DSTIP 1
        s4a += (char)0; // DSTIP 2
        s4a += (char)0; // DSTIP 3
        s4a += (char)1; // DSTIP 4
        s4a += (char)0; // NULL
        s4a += host_.hostname; // DOMAIN NAME
        s4a += (char)0; // NULL
                                
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(s4a),
            [this, self](boost::system::error_code ec, std::size_t){
                if (!ec){
                    get_s4_reply();
                }
            }
        );
    }

    void get_s4_reply(){
        memset(s4data_, 0, sizeof(s4data_));
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(s4data_, sizeof(s4data_)),
            [this, self](boost::system::error_code ec, std::size_t length){
                if (!ec){
                    if(s4data_[0] == 0){
                        do_read();
                    }
                    else{ // sock4 reply error
                        socket_.close();
                    }
                }
            }
        );
    }

    void do_read(){
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(rdata_, sizeof(rdata_)),
            [this, self](boost::system::error_code ec, std::size_t length){
                if (!ec){
                    string srdata_(rdata_);
                    sdata_ += srdata_;
                    memset(rdata_, 0, sizeof(rdata_));
                    
                    // print reply from remote
                    print_html_insertion(host_.id, replace_html_escape(sdata_), false);
                    fflush(stdout);
                    sdata_ = "";

                    string swdata_ = "";
                    if(find(srdata_.begin(), srdata_.end(), '%')!=srdata_.end()){ // found '%'
                        if(!fileline_.empty()){ // there are still some commands to do
                            strcpy(wdata_, ((*(fileline_.begin()))+"\r\n").c_str());
                            print_html_insertion(host_.id, replace_html_escape((*(fileline_.begin()))+"\n"), true);
                            fflush(stdout);
                            fileline_.erase(fileline_.begin());
                            do_write();
                        }
                    }
                    else{ // not found '%', keep reading
                        do_read();
                    }
                                
                }
            }
        );
    }

    void do_write(){
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

    static vector<string> readfile(string filename){
        fstream fs;
        fs.open(filename, ios::in);
        if(!fs.is_open()){
            cerr << "fs.open(): fail to open file." << endl;
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

    static void print_html_insertion(string id, string content, bool is_cmd){
        for(int i = 0; i < (int)content.size(); i++){
            if((int)content[i] < 32){
                content.erase(content.begin()+i);
                i--;
            }
        }

        cout << "<script>document.getElementById('" << id << "').innerHTML += '"
             <<(is_cmd ? "<b>":"")
             << content
             << (is_cmd ? "</b>":"")
             << "';</script>" <<endl;
        fflush(stdout);
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

void parse(string query_string, vector<Host>* host_list, SocksHost* sockshost){
    // parse query string
    vector<string> temp = {""};
    for(int i = 0; i < (int)query_string.size(); i++){
        if(query_string[i] == '&')
            temp.push_back("");
        else
            temp[temp.size()-1] += query_string[i];
    }

    int temp_size = (int)temp.size();

    // parse to host list
    if(host_list){
        host_list->clear();
        for(int i = 0; i < temp_size - 2; i += 3){
            if((int)temp[i].size() > 3 && (int)temp[i+1].size() > 3 && (int)temp[i+2].size() > 3){
                host_list->push_back(Host(
                    string(temp[i].begin()+3, temp[i].end()),
                    string(temp[i+1].begin()+3, temp[i+1].end()),
                    string(temp[i+2].begin()+3, temp[i+2].end()),
                    "s" + to_string(i)
                ));
            }
        }
    }

    // parse to socks list
    if(sockshost){
        sockshost->hostname = string(temp[temp_size-2].begin()+3, temp[temp_size-2].end());
        sockshost->port = string(temp[temp_size-1].begin()+3, temp[temp_size-1].end());
    }

}

int main(){
    string query_string = getenv("QUERY_STRING");
    vector<Host> host_list;
    SocksHost sockshost;
    parse(query_string, &host_list, &sockshost);

    cout << "Content-type: text/html\r\n\r\n" << endl;
    fflush(stdout);
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
    for(int i = 0; i < (int)html_base.size(); i++){
        if((int)html_base[i] < 32){
            html_base.erase(html_base.begin()+i);
            i--;
        }
    }
    cout << html_base << endl;
    fflush(stdout);

    for(int i = 0; i < (int)host_list.size(); i++){
        Client::print_html_insertion("s_title", "<th scope=\"col\">" + host_list[i].hostname + ":" + host_list[i].port + "</th>", false);
        Client::print_html_insertion("s_body", "<td><pre id=\"" + host_list[i].id + "\" class=\"mb-0\"></pre></td>", false);
    }
    fflush(stdout);

    boost::asio::io_context io_context;
    for(int i = 0; i < (int)host_list.size(); i++){
        tcp::resolver::query query(sockshost.hostname, sockshost.port);
        make_shared<Client>(io_context, move(query), host_list[i], sockshost)->start();
    }

    io_context.run();
    return 0;
}
