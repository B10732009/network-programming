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

typedef boost::asio::ip::tcp::socket boost_socket;
typedef boost::asio::ip::tcp::endpoint boost_endpoint;
typedef boost::asio::io_context boost_io_context;

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
    vector<string> fileline_;

public:
    Client(boost::asio::io_context& io_context_, tcp::resolver::query query_, Host host_):
            socket_(io_context_), resolver_(io_context_), query_(move(query_)), host_(host_){
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
                            if(!ec) {
                                do_read();
                            }
                        }
                    );
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
                    if(find(srdata_.begin(), srdata_.end(), '%')==srdata_.end()){ // not found '%'
                        do_read();
                    }
                    else{ // found '%'
                        print_html_insertion(host_.id, replace_html_escape(sdata_), false);//cout<<sdata_;
                        fflush(stdout);
                        sdata_ = "";

                        string swdata_ = "";
                        if(!fileline_.empty()){
                            strcpy(wdata_, ((*(fileline_.begin()))+"\r\n").c_str());
                            print_html_insertion(host_.id, replace_html_escape((*(fileline_.begin()))+"\n"), true);//cout<<(*(fileline_.begin()))+;
                            fflush(stdout);
                            fileline_.erase(fileline_.begin());
                        }
                        /*getline(cin, swdata_);
                        for(int i=0;i<(int)swdata_.size();i++){
                            if(swdata_[i]<32){
                                swdata_.erase(swdata_.begin()+i);
                                i--;
                            }
                        }
                        swdata_ += "\r\n";
                        strcpy(wdata_, swdata_.c_str());*/
                        do_write();
                    }
                }
            }
        );
    }

    void do_write(){
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(wdata_, strlen(wdata_)),
            [this, self](boost::system::error_code ec, std::size_t){
                if(!ec){
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
            cerr<<"fs.open(): fail to open file."<<endl;
            return {};
        }
        vector<string> fileline;
        string line;
        while(getline(fs, line)){
            for(int i = 0; i < (int)line.size(); i++){
                if(line[i] < 32){
                    line.erase(line.begin() + i);
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
                content.erase(content.begin() + i);
                i--;
            }
        }
        cout << "<script>document.getElementById('" << id << "').innerHTML += '"
            << (is_cmd?"<b>":"")
            << content
            << (is_cmd?"</b>":"")
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

int main(){
    string query_string = getenv("QUERY_STRING");
    vector<Host> host_list = parse(query_string);

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
        Client::print_html_insertion("s_title", "<th scope=\"col\">"+host_list[i].hostname+":"+host_list[i].port+"</th>", false);
        Client::print_html_insertion("s_body", "<td><pre id=\""+host_list[i].id+"\" class=\"mb-0\"></pre></td>", false);
    }
    fflush(stdout);

    boost::asio::io_context io_context;
    for(int i = 0; i < (int)host_list.size(); i++){
        tcp::resolver::query query(host_list[i].hostname, host_list[i].port);
        make_shared<Client>(io_context, move(query), host_list[i])->start();
    }

    io_context.run();
    return 0;
}



