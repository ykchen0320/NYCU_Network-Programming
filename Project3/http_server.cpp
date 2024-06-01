#include <sys/wait.h>
#include <unistd.h>

#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>

using boost::asio::ip::tcp;
std::unordered_map<std::string, std::string> envTable;

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

class session : public std::enable_shared_from_this<session> {
 public:
  session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start() { do_read(); }

 private:
  void do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            data_[length] = '\0';
            getEnv(data_);
            pid_t childpid = fork();
            while (childpid == -1) childpid = fork();
            if (childpid == 0) {
              envTable["SERVER_ADDR"] =
                  socket_.local_endpoint().address().to_string();
              envTable["SERVER_PORT"] =
                  std::to_string(socket_.local_endpoint().port());
              envTable["REMOTE_ADDR"] =
                  socket_.remote_endpoint().address().to_string();
              envTable["REMOTE_PORT"] =
                  std::to_string(socket_.remote_endpoint().port());
              for (auto& it : envTable) {
                setenv(it.first.c_str(), it.second.c_str(), 1);
              }
              dup2(socket_.native_handle(), STDIN_FILENO);
              dup2(socket_.native_handle(), STDOUT_FILENO);
              close(socket_.native_handle());
              std::cout << "HTTP/1.1 200 OK\r\n" << std::flush;
              int end = envTable["REQUEST_URI"].find("?");
              std::string filename =
                  "." + envTable["REQUEST_URI"].substr(0, end);
              char* argv1[] = {(char*)filename.c_str(), NULL};
              if (execv(filename.c_str(), argv1) == -1) exit(0);
            } else {
              initial();
              socket_.close();
              int status;
              waitpid(childpid, &status, 0);
            }
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