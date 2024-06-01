#include <sys/wait.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>
#define N_SERVERS 5
#define TEST_CASE_DIR "test_case"
using namespace boost::asio;
using namespace boost::asio::ip;
io_service ioservice;
std::string html_escape(const std::string &unescaped);
void get_QUERY_STRING();
void printHTML();
struct clientInfo {
  std::string hostname = "", port = "", file = "";
};
clientInfo info[N_SERVERS];
class Client : public std::enable_shared_from_this<Client> {
 public:
  Client(int id1, std::string hostname1, std::string port1, std::string file1)
      : id(id1), hostname(hostname1), port(port1), file(file1){};
  void start() {
    auto self(shared_from_this());
    open_file();
    // tcp::resolver::query q("localhost", "7099");
    tcp::resolver::query q(hostname, port);
    resolv.async_resolve(q, [this, self](const boost::system::error_code ec,
                                         tcp::resolver::iterator it) {
      if (!ec) {
        tcp_socket.async_connect(
            *it, [this, self](const boost::system::error_code ec) {
              if (!ec) {
                do_read();
              }
            });
      }
    });
  }

 private:
  void open_file() {
    iffile.open("./" + std::string(TEST_CASE_DIR) + "/" + file);
  }
  void do_read() {
    auto self(shared_from_this());
    content = "";
    bzero(data_, max_length);
    tcp_socket.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](const boost::system::error_code &ec, std::size_t) {
          if (!ec) {
            content = std::string(data_);
            std::cout << "<script>document.getElementById('s"
                      << std::to_string(id) << "').innerHTML += '"
                      << html_escape(content) << "';</script>" << std::flush;
            if (content.find("%") != std::string::npos) {
              do_write();
            } else {
              do_read();
            }
          }
        });
  }
  void do_write() {
    auto self(shared_from_this());
    content = "";
    getline(iffile, content);
    content += "\n";
    std::cout << "<script>document.getElementById('s" << std::to_string(id)
              << "').innerHTML += '<b>" << html_escape(content)
              << "</b>';</script>" << std::flush;
    async_write(tcp_socket, buffer(content, strlen(content.c_str())),
                [this, self](const boost::system::error_code &ec,
                             std::size_t /*length*/) {
                  if (!ec) {
                    if (content.find("exit") != std::string::npos) {
                      iffile.close();
                      info[id].hostname = "";
                    }
                    content = "";
                    if (info[id].hostname != "") {
                      do_read();
                    }
                  }
                });
  }
  int id;
  std::string hostname = "", port = "", file = "", content = "";
  tcp::socket tcp_socket{ioservice};
  tcp::resolver resolv{ioservice};
  std::ifstream iffile;
  enum { max_length = 102400 };
  char data_[max_length];
};
std::string html_escape(const std::string &unescaped) {
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
void get_QUERY_STRING() {
  char *qstr = getenv("QUERY_STRING");
  std::string str = std::string(qstr);
  std::size_t start, end;
  for (int i = 0; i < N_SERVERS; i++) {
    start = str.find("=");
    end = str.find("&");
    info[i].hostname = str.substr(start + 1, end - start - 1);
    str = str.substr(end + 1);
    start = str.find("=");
    end = str.find("&");
    info[i].port = str.substr(start + 1, end - start - 1);
    str = str.substr(end + 1);
    start = str.find("=");
    end = str.find("&");
    if (end == std::string::npos) {
      info[i].file = str.substr(start + 1);
    } else {
      info[i].file = str.substr(start + 1, end - start - 1);
    }
    str = str.substr(end + 1);
  }
  return;
}
void printHTML() {
  std::cout << "Content-type: text/html\r\n\r\n" << std::flush;
  std::string html_content =
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
      "<link\n"
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
    if (info[i].hostname != "") {
      html_content = html_content + "<th scope=\"col\">" + info[i].hostname +
                     ":" + info[i].port + "</th>\n";
    }
  }
  html_content = html_content +
                 "</tr>\n"
                 "</thead>\n"
                 "<tbody>\n"
                 "<tr>\n";
  for (int i = 0; i < N_SERVERS; i++) {
    if (info[i].hostname != "") {
      html_content = html_content + "<td><pre id=\"s" + std::to_string(i) +
                     "\" class=\"mb-0\">"
                     "</pre></td>\n";
    }
  }
  html_content = html_content +
                 "</tr>\n"
                 "</tbody>\n"
                 "</table>\n"
                 "</body>\n"
                 "</html>\n";
  std::cout << html_content << std::flush;
}
int main() {
  try {
    get_QUERY_STRING();
    printHTML();
    for (int i = 0; i < N_SERVERS; i++) {
      if (info[i].hostname != "") {
        std::make_shared<Client>(i, info[i].hostname, info[i].port,
                                 info[i].file)
            ->start();
      }
    }
    ioservice.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}