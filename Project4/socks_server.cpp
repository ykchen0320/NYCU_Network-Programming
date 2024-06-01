#include <signal.h>
#include <sys/wait.h>

#include <boost/asio.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define filename "./socks.conf"

using boost::asio::ip::tcp;

class Info {
 public:
  int vn, cd;
  std::string srcIP, srcPort, dstIP, dstPort, cmd, reply;
};

boost::asio::io_context io_context;

void signal_handler(int signo) {
  int status;
  while (waitpid(-1, &status, WNOHANG) > 0) {
  }
}

class session : public std::enable_shared_from_this<session> {
 public:
  session(tcp::socket socket)
      : srcSocket(std::move(socket)), dstSocket(io_context) {}

  void start() { socks4_request(); }

 private:
  std::vector<std::string> string_slice(std::string str1) {
    std::vector<std::string> vec1;
    int end = str1.find(" ");
    vec1.push_back(str1.substr(0, end));
    str1 = str1.substr(end + 1);
    end = str1.find(" ");
    vec1.push_back(str1.substr(0, end));
    str1 = str1.substr(end + 1);
    end = str1.find(" ");
    vec1.push_back(str1.substr(0));
    return vec1;
  }

  void firewall_check() {
    info.reply = "Reject";
    std::fstream file;
    file.open(filename);
    std::string str;
    while (getline(file, str)) {
      if (info.cmd == "CONNECT") {
        if (str.find(" c ") == std::string::npos) continue;
      } else if (info.cmd == "BIND") {
        if (str.find(" b ") == std::string::npos) continue;
      }
      std::vector<std::string> vec = string_slice(str);
      str = info.dstIP;
      std::string str1 = vec[2];
      int flag = 0;
      for (int i = 0; i < 4; i++) {
        std::string str2 = str.substr(0, str.find('.'));
        std::string str3 = str1.substr(0, str1.find('.'));
        if (str3 == "*") {
          flag = 1;
          break;
        } else if (str2 == str3) {
          str = str.substr(str.find('.') + 1);
          str1 = str1.substr(str1.find('.') + 1);
          if (i == 3) flag = 1;
          continue;
        } else {
          flag = 0;
          break;
        }
      }
      if (flag) {
        info.reply = "Accept";
      }
    }
    file.close();
  }

  void do_write_to_src(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(
        srcSocket, boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, size_t length) {
          if (!ec) {
            do_read_from_dst();
          } else {
            close_sockets();
          }
        });
  }

  void do_read_from_dst() {
    auto self(shared_from_this());
    memset(data_, '\0', sizeof(data_));
    dstSocket.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            do_write_to_src(length);
          } else {
            close_sockets();
          }
        });
  }

  void do_write_to_dst(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(
        dstSocket, boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, size_t length) {
          if (!ec) {
            do_read_from_src();
          } else {
            close_sockets();
          }
        });
  }

  void do_read_from_src() {
    auto self(shared_from_this());
    memset(data_, '\0', sizeof(data_));
    srcSocket.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            do_write_to_dst(length);
          } else {
            close_sockets();
          }
        });
  }

  void socks4_reply() {
    auto self(shared_from_this());
    boost::asio::async_write(
        srcSocket, boost::asio::buffer(reply_message),
        [this, self](boost::system::error_code ec, size_t length) {
          if (ec) {
            close_sockets();
          }
        });
  }

  void do_connect() {
    auto self(shared_from_this());
    tcp::resolver::query q(info.dstIP, info.dstPort);
    resolv.async_resolve(q, [this, self](const boost::system::error_code ec,
                                         tcp::resolver::iterator it) {
      if (!ec) {
        dstSocket.async_connect(
            *it, [this, self](const boost::system::error_code ec) {
              if (!ec) {
                reply_message[1] = 90;
                socks4_reply();
                do_read_from_src();
                do_read_from_dst();
              } else {
                std::cerr << "Connect error: " << ec.message() << " ("
                          << ec.value() << ")" << std::endl;
                reply_message[1] = 91;
                socks4_reply();
                close_sockets();
              }
            });
      } else {
        std::cerr << "Resolve error: " << ec.message() << " (" << ec.value()
                  << ")" << std::endl;
        reply_message[1] = 91;
        socks4_reply();
        close_sockets();
      }
    });
  }

  void do_bind() {
    auto self(shared_from_this());
    tcp::acceptor acceptor1(io_context, tcp::endpoint(tcp::v4(), 0));
    unsigned int port = acceptor1.local_endpoint().port();
    reply_message[1] = 90;
    reply_message[2] = port / 256;
    reply_message[3] = port % 256;
    socks4_reply();
    acceptor1.accept(dstSocket);
    socks4_reply();
    do_read_from_src();
    do_read_from_dst();
  }

  void socks4_request() {
    auto self(shared_from_this());
    memset(data_, '\0', sizeof(data_));
    memset(reply_message, 0, sizeof(reply_message));
    srcSocket.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            info.srcIP = srcSocket.remote_endpoint().address().to_string();
            info.srcPort = std::to_string(srcSocket.remote_endpoint().port());
            info.vn = data_[0];
            info.cd = data_[1];
            info.dstPort = std::to_string((data_[2] << 8) | data_[3]);
            info.dstIP =
                std::to_string(data_[4]) + "." + std::to_string(data_[5]) +
                "." + std::to_string(data_[6]) + "." + std::to_string(data_[7]);
            if (data_[4] == 0 && data_[5] == 0 && data_[6] == 0 &&
                data_[7] != 0) {
              int index = 8;
              while (data_[index] != 0) index++;
              index++;
              while (data_[index] != 0) {
                domainName += data_[index];
                index++;
              }
              tcp::resolver resolv(io_context);
              tcp::resolver::query q(domainName, info.dstPort);
              tcp::resolver::iterator it;
              for (it = resolv.resolve(q); it != tcp::resolver::iterator();
                   ++it) {
                tcp::endpoint endpoint = *it;
                if (endpoint.address().is_v4()) {
                  info.dstIP = endpoint.address().to_string();
                }
              }
            }
            if (info.vn == 4) {
              info.reply = "Accept";
            } else {
              info.reply = "Reject";
            }
            if (info.cd == 1) {
              info.cmd = "CONNECT";
            } else {
              info.cmd = "BIND";
            }
            firewall_check();
            if (info.reply == "Accept") {
              std::cout << "<S_IP>: " << info.srcIP << std::endl;
              std::cout << "<S_PORT>: " << info.srcPort << std::endl;
              std::cout << "<D_IP>: " << info.dstIP << std::endl;
              std::cout << "<D_PORT>: " << info.dstPort << std::endl;
              std::cout << "<Command>: " << info.cmd << std::endl;
              std::cout << "<Reply>: " << info.reply << std::endl;
              if (info.cmd == "CONNECT") {
                do_connect();
              } else {
                do_bind();
              }
            } else {
              reply_message[1] = 91;
              socks4_reply();
              close_sockets();
            }
          } else {
            std::cerr << "Request error: " << ec.message() << " (" << ec.value()
                      << ")" << std::endl;
            close_sockets();
          }
        });
  }

  void close_sockets() {
    if (srcSocket.is_open()) srcSocket.close();
    if (dstSocket.is_open()) dstSocket.close();
    exit(0);
  }

  tcp::socket srcSocket, dstSocket;
  tcp::resolver resolv{io_context};
  enum { max_length = 102400 };
  unsigned char data_[max_length], reply_message[8];
  Info info;
  std::string domainName = "", userID = "";
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
            signal(SIGCHLD, signal_handler);
            io_context.notify_fork(boost::asio::io_service::fork_prepare);
            pid_t childPid = fork();
            while (childPid < 0) {
              childPid = fork();
            }
            if (childPid == 0) {
              io_context.notify_fork(boost::asio::io_service::fork_child);
              acceptor_.close();
              std::make_shared<session>(std::move(socket))->start();
            } else {
              io_context.notify_fork(boost::asio::io_service::fork_parent);
              socket.close();
            }
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: socks4_server <port>\n";
      return 1;
    }

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
