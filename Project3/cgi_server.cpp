#include <boost/asio.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>
#define N_SERVERS 5
#define TEST_CASE_DIR "test_case"
#define MAX_LENGTH 102400
using boost::asio::ip::tcp;
using namespace boost::asio;
using namespace boost::asio::ip;
std::unordered_map<std::string, std::string> envTable;
class Client {
 public:
  std::string hostname = "", port = "", file = "";
  io_service ioservice;
  tcp::socket tcp_socket{ioservice};
  tcp::resolver resolv{ioservice};
  std::ifstream iffile;
  void reset() {
    hostname = "";
    port = "";
    file = "";
  }
};
const std::string panelHTTP =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "<title>NP Project 3 Panel</title>\n"
    "<link\n"
    "rel=\"stylesheet\"\n"
    "href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/"
    "bootstrap.min.css\"\n"
    "integrity=\"sha384-TX8t27EcRE3e/"
    "ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
    "crossorigin=\"anonymous\"\n"
    "/>\n"
    "<link\n"
    "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
    "rel=\"stylesheet\"\n"
    "/>\n"
    "<link\n"
    "rel=\"icon\"\n"
    "type=\"image/png\"\n"
    "href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/"
    "512/"
    "dashboard-512.png\"\n"
    "/>\n"
    "<style>\n"
    "* {\n"
    "font-family: \'Source Code Pro\', monospace;\n"
    "}\n"
    "</style>\n"
    "</head>\n"
    "<body class=\"bg-secondary pt-5\">\n"
    "<form action=\"console.cgi\" method=\"GET\">\n"
    "<table class=\"table mx-auto bg-light\" style=\"width: inherit\">\n"
    "<thead class=\"thead-dark\">\n"
    "<tr>\n"
    "<th scope=\"col\">#</th>\n"
    "<th scope=\"col\">Host</th>\n"
    "<th scope=\"col\">Port</th>\n"
    "<th scope=\"col\">Input File</th>\n"
    "</tr>\n"
    "</thead>\n"
    "<tbody>\n"
    "<tr>\n"
    "<th scope=\"row\" class=\"align-middle\">Session 1</th>\n"
    "<td>\n"
    "<div class=\"input-group\">\n"
    "<select name=\"h0\" class=\"custom-select\">\n"
    "<option></option><option "
    "value=\"nplinux1.cs.nycu.edu.tw\">nplinux1</option><option "
    "value=\"nplinux2.cs.nycu.edu.tw\">nplinux2</option><option "
    "value=\"nplinux3.cs.nycu.edu.tw\">nplinux3</option><option "
    "value=\"nplinux4.cs.nycu.edu.tw\">nplinux4</option><option "
    "value=\"nplinux5.cs.nycu.edu.tw\">nplinux5</option><option "
    "value=\"nplinux6.cs.nycu.edu.tw\">nplinux6</option><option "
    "value=\"nplinux7.cs.nycu.edu.tw\">nplinux7</option><option "
    "value=\"nplinux8.cs.nycu.edu.tw\">nplinux8</option><option "
    "value=\"nplinux9.cs.nycu.edu.tw\">nplinux9</option><option "
    "value=\"nplinux10.cs.nycu.edu.tw\">nplinux10</option><option "
    "value=\"nplinux11.cs.nycu.edu.tw\">nplinux11</option><option "
    "value=\"nplinux12.cs.nycu.edu.tw\">nplinux12</option>\n"
    "</select>\n"
    "<div class=\"input-group-append\">\n"
    "<span class=\"input-group-text\">.cs.nycu.edu.tw</span>\n"
    "</div>\n"
    "</div>\n"
    "</td>\n"
    "<td>\n"
    "<input name=\"p0\" type=\"text\" class=\"form-control\" size=\"5\" "
    "/>\n"
    " </td>\n"
    "<td>\n"
    "<select name=\"f0\" class=\"custom-select\">\n"
    "<option></option>\n"
    "<option value=\"t1.txt\">t1.txt</option><option "
    "value=\"t2.txt\">t2.txt</option><option "
    "value=\"t3.txt\">t3.txt</option><option "
    "value=\"t4.txt\">t4.txt</option><option "
    "value=\"t5.txt\">t5.txt</option>\n"
    "</select>\n"
    " </td>\n"
    "</tr>\n"
    "<tr>\n"
    "<th scope=\"row\" class=\"align-middle\">Session 2</th>\n"
    "<td>\n"
    "<div class=\"input-group\">\n"
    "<select name=\"h1\" class=\"custom-select\">\n"
    "<option></option><option "
    "value=\"nplinux1.cs.nycu.edu.tw\">nplinux1</option><option "
    "value=\"nplinux2.cs.nycu.edu.tw\">nplinux2</option><option "
    "value=\"nplinux3.cs.nycu.edu.tw\">nplinux3</option><option "
    "value=\"nplinux4.cs.nycu.edu.tw\">nplinux4</option><option "
    "value=\"nplinux5.cs.nycu.edu.tw\">nplinux5</option><option "
    "value=\"nplinux6.cs.nycu.edu.tw\">nplinux6</option><option "
    "value=\"nplinux7.cs.nycu.edu.tw\">nplinux7</option><option "
    "value=\"nplinux8.cs.nycu.edu.tw\">nplinux8</option><option "
    "value=\"nplinux9.cs.nycu.edu.tw\">nplinux9</option><option "
    "value=\"nplinux10.cs.nycu.edu.tw\">nplinux10</option><option "
    "value=\"nplinux11.cs.nycu.edu.tw\">nplinux11</option><option "
    "value=\"nplinux12.cs.nycu.edu.tw\">nplinux12</option>\n"
    "</select>\n"
    "<div class=\"input-group-append\">\n"
    "<span class=\"input-group-text\">.cs.nycu.edu.tw</span>\n"
    "</div>\n"
    "</div>\n"
    "</td>\n"
    "<td>\n"
    "<input name=\"p1\" type=\"text\" class=\"form-control\" size=\"5\" "
    "/>\n"
    " </td>\n"
    "<td>\n"
    "<select name=\"f1\" class=\"custom-select\">\n"
    "<option></option>\n"
    "<option value=\"t1.txt\">t1.txt</option><option "
    "value=\"t2.txt\">t2.txt</option><option "
    "value=\"t3.txt\">t3.txt</option><option "
    "value=\"t4.txt\">t4.txt</option><option "
    "value=\"t5.txt\">t5.txt</option>\n"
    "</select>\n"
    " </td>\n"
    "</tr>\n"
    "<tr>\n"
    "<th scope=\"row\" class=\"align-middle\">Session 3</th>\n"
    "<td>\n"
    "<div class=\"input-group\">\n"
    "<select name=\"h2\" class=\"custom-select\">\n"
    "<option></option><option "
    "value=\"nplinux1.cs.nycu.edu.tw\">nplinux1</option><option "
    "value=\"nplinux2.cs.nycu.edu.tw\">nplinux2</option><option "
    "value=\"nplinux3.cs.nycu.edu.tw\">nplinux3</option><option "
    "value=\"nplinux4.cs.nycu.edu.tw\">nplinux4</option><option "
    "value=\"nplinux5.cs.nycu.edu.tw\">nplinux5</option><option "
    "value=\"nplinux6.cs.nycu.edu.tw\">nplinux6</option><option "
    "value=\"nplinux7.cs.nycu.edu.tw\">nplinux7</option><option "
    "value=\"nplinux8.cs.nycu.edu.tw\">nplinux8</option><option "
    "value=\"nplinux9.cs.nycu.edu.tw\">nplinux9</option><option "
    "value=\"nplinux10.cs.nycu.edu.tw\">nplinux10</option><option "
    "value=\"nplinux11.cs.nycu.edu.tw\">nplinux11</option><option "
    "value=\"nplinux12.cs.nycu.edu.tw\">nplinux12</option>\n"
    "</select>\n"
    "<div class=\"input-group-append\">\n"
    "<span class=\"input-group-text\">.cs.nycu.edu.tw</span>\n"
    "</div>\n"
    "</div>\n"
    "</td>\n"
    "<td>\n"
    "<input name=\"p2\" type=\"text\" class=\"form-control\" size=\"5\" "
    "/>\n"
    " </td>\n"
    "<td>\n"
    "<select name=\"f2\" class=\"custom-select\">\n"
    "<option></option>\n"
    "<option value=\"t1.txt\">t1.txt</option><option "
    "value=\"t2.txt\">t2.txt</option><option "
    "value=\"t3.txt\">t3.txt</option><option "
    "value=\"t4.txt\">t4.txt</option><option "
    "value=\"t5.txt\">t5.txt</option>\n"
    "</select>\n"
    " </td>\n"
    "</tr>\n"
    "<tr>\n"
    "<th scope=\"row\" class=\"align-middle\">Session 4</th>\n"
    "<td>\n"
    "<div class=\"input-group\">\n"
    "<select name=\"h3\" class=\"custom-select\">\n"
    "<option></option><option "
    "value=\"nplinux1.cs.nycu.edu.tw\">nplinux1</option><option "
    "value=\"nplinux2.cs.nycu.edu.tw\">nplinux2</option><option "
    "value=\"nplinux3.cs.nycu.edu.tw\">nplinux3</option><option "
    "value=\"nplinux4.cs.nycu.edu.tw\">nplinux4</option><option "
    "value=\"nplinux5.cs.nycu.edu.tw\">nplinux5</option><option "
    "value=\"nplinux6.cs.nycu.edu.tw\">nplinux6</option><option "
    "value=\"nplinux7.cs.nycu.edu.tw\">nplinux7</option><option "
    "value=\"nplinux8.cs.nycu.edu.tw\">nplinux8</option><option "
    "value=\"nplinux9.cs.nycu.edu.tw\">nplinux9</option><option "
    "value=\"nplinux10.cs.nycu.edu.tw\">nplinux10</option><option "
    "value=\"nplinux11.cs.nycu.edu.tw\">nplinux11</option><option "
    "value=\"nplinux12.cs.nycu.edu.tw\">nplinux12</option>\n"
    "</select>\n"
    "<div class=\"input-group-append\">\n"
    "<span class=\"input-group-text\">.cs.nycu.edu.tw</span>\n"
    "</div>\n"
    "</div>\n"
    "</td>\n"
    "<td>\n"
    "<input name=\"p3\" type=\"text\" class=\"form-control\" size=\"5\" "
    "/>\n"
    " </td>\n"
    "<td>\n"
    "<select name=\"f3\" class=\"custom-select\">\n"
    "<option></option>\n"
    "<option value=\"t1.txt\">t1.txt</option><option "
    "value=\"t2.txt\">t2.txt</option><option "
    "value=\"t3.txt\">t3.txt</option><option "
    "value=\"t4.txt\">t4.txt</option><option "
    "value=\"t5.txt\">t5.txt</option>\n"
    "</select>\n"
    " </td>\n"
    "</tr>\n"
    "<tr>\n"
    "<th scope=\"row\" class=\"align-middle\">Session 5</th>\n"
    "<td>\n"
    "<div class=\"input-group\">\n"
    "<select name=\"h4\" class=\"custom-select\">\n"
    "<option></option><option "
    "value=\"nplinux1.cs.nycu.edu.tw\">nplinux1</option><option "
    "value=\"nplinux2.cs.nycu.edu.tw\">nplinux2</option><option "
    "value=\"nplinux3.cs.nycu.edu.tw\">nplinux3</option><option "
    "value=\"nplinux4.cs.nycu.edu.tw\">nplinux4</option><option "
    "value=\"nplinux5.cs.nycu.edu.tw\">nplinux5</option><option "
    "value=\"nplinux6.cs.nycu.edu.tw\">nplinux6</option><option "
    "value=\"nplinux7.cs.nycu.edu.tw\">nplinux7</option><option "
    "value=\"nplinux8.cs.nycu.edu.tw\">nplinux8</option><option "
    "value=\"nplinux9.cs.nycu.edu.tw\">nplinux9</option><option "
    "value=\"nplinux10.cs.nycu.edu.tw\">nplinux10</option><option "
    "value=\"nplinux11.cs.nycu.edu.tw\">nplinux11</option><option "
    "value=\"nplinux12.cs.nycu.edu.tw\">nplinux12</option>\n"
    "</select>\n"
    "<div class=\"input-group-append\">\n"
    "<span class=\"input-group-text\">.cs.nycu.edu.tw</span>\n"
    "</div>\n"
    "</div>\n"
    "</td>\n"
    "<td>\n"
    "<input name=\"p4\" type=\"text\" class=\"form-control\" size=\"5\" "
    "/>\n"
    " </td>\n"
    "<td>\n"
    "<select name=\"f4\" class=\"custom-select\">\n"
    "<option></option>\n"
    "<option value=\"t1.txt\">t1.txt</option><option "
    "value=\"t2.txt\">t2.txt</option><option "
    "value=\"t3.txt\">t3.txt</option><option "
    "value=\"t4.txt\">t4.txt</option><option "
    "value=\"t5.txt\">t5.txt</option>\n"
    "</select>\n"
    " </td>\n"
    "</tr>\n"
    "<tr>\n"
    "<td colspan=\"3\"></td>\n"
    "<td>\n"
    "<button type=\"submit\" class=\"btn btn-info "
    "btn-block\">Run</button>\n"
    "</td>\n"
    "</tr>\n"
    "</tbody>\n"
    "</table>\n"
    "</form>\n"
    "</body>\n"
    "</html>\n";
int id;
std::string content = "";
Client clients[N_SERVERS];
void get_QUERY_STRING() {
  std::string str = envTable["QUERY_STRING"];
  std::size_t start, end;
  for (int i = 0; i < N_SERVERS; i++) {
    start = str.find("=");
    end = str.find("&");
    clients[i].hostname = str.substr(start + 1, end - start - 1);
    str = str.substr(end + 1);
    start = str.find("=");
    end = str.find("&");
    clients[i].port = str.substr(start + 1, end - start - 1);
    str = str.substr(end + 1);
    start = str.find("=");
    end = str.find("&");
    if (end == std::string::npos) {
      clients[i].file = str.substr(start + 1);
    } else {
      clients[i].file = str.substr(start + 1, end - start - 1);
    }
    str = str.substr(end + 1);
  }
  return;
}
void initial() {
  envTable["REQUEST_METHOD"] = "";
  envTable["REQUEST_URI"] = "";
  envTable["QUERY_STRING"] = "";
  envTable["SERVER_PROTOCOL"] = "";
  envTable["HTTP_HOST"] = "";
  envTable["SERVER_ADDR"] = "";
  envTable["SERVER_PORT"] = "";
  envTable["REMOTE_ADDR"] = "";
  envTable["REMOTE_PORT"] = "";
}
void getEnv(char* data) {
  std::string origin_str(data);
  std::string str = origin_str;
  envTable["REQUEST_METHOD"] = "GET";
  std::size_t start, end;
  str = str.substr(4);
  start = str.find("/");
  end = str.find(" ");
  if (start != std::string::npos && end != std::string::npos) {
    envTable["REQUEST_URI"] = str.substr(start, end - start);
  }
  start = str.find("?");
  end = str.find(" ");
  if (start != std::string::npos && end != std::string::npos) {
    envTable["QUERY_STRING"] = str.substr(start + 1, end - start - 1);
  }
  str = str.substr(end);
  start = str.find(" ");
  end = str.find("\n");
  if (start != std::string::npos && end != std::string::npos) {
    envTable["SERVER_PROTOCOL"] = str.substr(start + 1, end - start - 1);
  }
  str = str.substr(end);
  start = str.find("HOST");
  end = str.find(" ");
  str = str.substr(end);
  start = str.find(" ");
  end = str.find("\n");
  if (start != std::string::npos && end != std::string::npos) {
    envTable["HTTP_HOST"] = str.substr(start + 1, end - start - 1);
  }
}
std::string html_escape(const std::string& unescaped) {
  std::string escaped;
  for (char c : unescaped) {
    switch (c) {
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '&':
        escaped += "&amp;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&#39;";
        break;
      case '\n':
        escaped += "&NewLine;";
        break;
      case '\r':
        escaped += "";
        break;
      default:
        escaped += c;
        break;
    }
  }
  return escaped;
}
class session : public std::enable_shared_from_this<session> {
 public:
  session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start() { do_read(); }

 private:
  void print_panel_HTML() { do_write(panelHTTP); }
  void print_console_HTML() {
    std::string consoleHTML =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\" />\n"
        "<title>NP Project 3 Sample Console</title>\n"
        "<link\n"
        "rel=\"stylesheet\"\n"
        "href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/"
        "bootstrap.min.css\"\n"
        "integrity=\"sha384-TX8t27EcRE3e/"
        "ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
        "crossorigin=\"anonymous\"\n"
        "/>"
        "<link\n"
        "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
        "rel=\"stylesheet\"\n"
        "/>\n"
        "rel=\"icon\"\n"
        "type=\"image/png\"\n"
        "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/"
        "678068-terminal-512.png\"\n"
        "/>\n"
        "<style>\n"
        "* {\n"
        "font-family: 'Source Code Pro', monospace;\n"
        "font-size: 1rem !important;\n"
        "}\n"
        "body {\n"
        "background-color: #212529;\n"
        "}\n"
        "pre {\n"
        "color: #cccccc;\n"
        "}\n"
        "b {\n"
        "color: #01b468;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<table class=\"table table-dark table-bordered\">\n"
        "<thead>\n"
        "<tr>\n";
    for (int i = 0; i < N_SERVERS; i++) {
      if (clients[i].hostname != "") {
        consoleHTML = consoleHTML + "<th scope=\"col\">" + clients[i].hostname +
                      ":" + clients[i].port + "</th>\n";
      }
    }
    consoleHTML = consoleHTML +
                  "</tr>\n"
                  "</thead>\n"
                  "<tbody>\n"
                  "<tr>\n";
    for (int i = 0; i < N_SERVERS; i++) {
      if (clients[i].hostname != "") {
        consoleHTML = consoleHTML + "<td><pre id=\"s" + std::to_string(i) +
                      "\" class=\"mb-0\">"
                      "</pre></td>\n";
      }
    }
    consoleHTML = consoleHTML +
                  "</tr>\n"
                  "</tbody>\n"
                  "</table>\n"
                  "</body>\n"
                  "</html>\n";
    do_write(consoleHTML);
  }
  void do_console_read() {
    char data_[MAX_LENGTH];
    content = "";
    memset(data_, 0, MAX_LENGTH);
    clients[id].tcp_socket.async_read_some(
        boost::asio::buffer(data_, MAX_LENGTH),
        [&data_, this](const boost::system::error_code& ec, std::size_t) {
          if (!ec) {
            content = html_escape(std::string(data_));
            content = "<script>document.getElementById('s" +
                      std::to_string(id) + "').innerHTML += '" + content +
                      "';</script>";
            do_write(content);
            if (content.find("%") != std::string::npos) {
              do_console_write();
            } else {
              do_console_read();
            }
          }
        });
  }
  void do_console_write() {
    content = "";
    getline(clients[id].iffile, content);
    content += "\n";
    do_write("<script>document.getElementById('s" + std::to_string(id) +
             "').innerHTML += '<b>" + html_escape(content) + "</b>';</script>");
    async_write(
        clients[id].tcp_socket, buffer(content, strlen(content.c_str())),
        [this](const boost::system::error_code& ec, std::size_t length) {
          if (!ec) {
            if (content.find("exit") != std::string::npos) {
              clients[id].iffile.close();
              clients[id].reset();
            }
            content = "";
            do_console_read();
          }
        });
  }
  void do_console() {
    get_QUERY_STRING();
    print_console_HTML();
    for (id = 0; id < N_SERVERS; id++) {
      if (clients[id].hostname != "") {
        clients[id].iffile.open("./" + std::string(TEST_CASE_DIR) + "/" +
                                clients[id].file);
        tcp::resolver::query q(clients[id].hostname, clients[id].port);
        clients[id].resolv.async_resolve(
            q, [this](const boost::system::error_code ec,
                      tcp::resolver::iterator it) {
              if (!ec) {
                clients[id].tcp_socket.async_connect(
                    *it, [this](const boost::system::error_code ec) {
                      if (!ec) {
                        do_console_read();
                      }
                    });
              }
            });
        clients[id].ioservice.run();
      }
    }
  }
  void do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            data_[length] = '\0';
            getEnv(data_);
            envTable["SERVER_ADDR"] =
                socket_.local_endpoint().address().to_string();
            envTable["SERVER_PORT"] =
                std::to_string(socket_.local_endpoint().port());
            envTable["REMOTE_ADDR"] =
                socket_.remote_endpoint().address().to_string();
            envTable["REMOTE_PORT"] =
                std::to_string(socket_.remote_endpoint().port());
            do_write("HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n");
            int end = envTable["REQUEST_URI"].find("?");
            std::string filename = "." + envTable["REQUEST_URI"].substr(0, end);
            if (filename == "./panel.cgi") {
              print_panel_HTML();
            } else if (filename == "./console.cgi") {
              do_console();
            }
            socket_.close();
          }
        });
  }
  void do_write(std::string str) {
    async_write(socket_, buffer(str, strlen(str.c_str())),
                [](const boost::system::error_code& ec, std::size_t length) {
                  if (!ec) {
                  }
                });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server {
 public:
  server(boost::asio::io_context& io_context, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    do_accept();
  }

 private:
  void do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: async_tcp_http_server <port>\n";
      return 1;
    }
    initial();
    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}