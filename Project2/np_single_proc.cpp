#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#define PIPE_READ 0
#define PIPE_WRITE 1
#define QLEN 30
#define MAX_CLIENTS 30
class Cmd {
 public:
  std::vector<std::string> argv;
  bool file_tag = false, lpipe = false;
  int npipe = 0, in = 0, out = 0, err = 0, upipe_read = 0, upipe_write = 0,
      dev_null = 0;
  void reset() {
    argv.clear();
    file_tag = false;
    lpipe = false;
    npipe = 0;
    in = 0;
    out = 0;
    err = 0;
    upipe_read = 0;
    upipe_write = 0;
    dev_null = 0;
  }
};
class Pipe {
 public:
  int fd[2] = {0, 0};
  int counter = 0;
};
class Upipe {
 public:
  int fd[2] = {0, 0}, src = 0, des = 0;
};
class Client {
 public:
  int ssock = -1;
  std::string ip = "", port = "", nickname = "(no name)";
  bool print_mod = false;
  std::map<std::string, std::string> env;
  std::vector<Pipe> pipes;
  void reset() {
    ssock = -1;
    nickname = "(no name)";
    print_mod = false;
    ip = "";
    port = "";
    env.clear();
    pipes.clear();
  }
};
std::vector<Cmd> cmds;
std::vector<Upipe> upipes;
std::string str_cmd;
Client clients[MAX_CLIENTS + 1];
int nfds;
fd_set rfds, afds;
const char *mod_message = "% ";
static void signal_handler(int sig) {
  if (sig == SIGCHLD) {
    int wait_temp;
    while (waitpid(-1, &wait_temp, WNOHANG) > 0) {
    }
  }
}
void initial() {
  signal(SIGCHLD, signal_handler);
  setenv("PATH", "bin:.", 1);
  return;
}
void broadcast(std::string str) {
  const char *buf = str.c_str();
  for (int i = 1; i < MAX_CLIENTS + 1; i++) {
    if (clients[i].ssock != -1) {
      write(clients[i].ssock, (const void *)buf, sizeof(char) * str.size());
    }
  }
}
void cmd_slice(std::string str) {
  std::string token = "";
  std::vector<std::string> temp;
  for (int i = 0; i < str.size(); i++) {
    if (str[i] != ' ')
      token += str[i];
    else if (token != "") {
      temp.push_back(token);
      token = "";
    }
  }
  if (token.size()) temp.push_back(token);
  Cmd cmd_temp;
  for (int i = 0; i < temp.size(); i++) {
    cmd_temp.argv.push_back(temp[i]);
    if (temp[i][0] == '|' || temp[i][0] == '!' || temp[i][0] == '>' ||
        temp[i][0] == '<') {
      if (temp[i] == ">") {
        cmd_temp.file_tag = true;
      } else {
        bool upipe_tag = false;
        if (temp[i][0] == '!') cmd_temp.err = 1;
        if (temp[i].size() > 1) {
          if (temp[i][0] == '|') {
            temp[i].erase(0, 1);
            cmd_temp.npipe = stoi(temp[i]);
          } else if (temp[i][0] == '<') {
            temp[i].erase(0, 1);
            cmd_temp.upipe_read = stoi(temp[i]);
            upipe_tag = true;
          } else if (temp[i][0] == '>') {
            temp[i].erase(0, 1);
            cmd_temp.upipe_write = stoi(temp[i]);
            upipe_tag = true;
          }
        } else {
          cmd_temp.lpipe = true;
        }
        if (upipe_tag) {
          cmd_temp.argv.pop_back();
        } else {
          cmd_temp.argv.pop_back();
          cmds.push_back(cmd_temp);
          cmd_temp.reset();
        }
      }
    }
  }
  if (cmd_temp.argv.size()) cmds.push_back(cmd_temp);
}
void input_check(int id, int i) {
  if (cmds[i].upipe_read) {
    if (cmds[i].upipe_read < 1 || cmds[i].upipe_read > MAX_CLIENTS ||
        clients[cmds[i].upipe_read].ssock == -1) {
      cmds[i].dev_null = open("/dev/null", O_RDWR);
      cmds[i].in = cmds[i].dev_null;
      std::string str = "*** Error: user #" +
                        std::to_string(cmds[i].upipe_read) +
                        " does not exist yet. ***";
      str.push_back('\n');
      const char *buf = str.c_str();
      write(clients[id].ssock, (const char *)buf, sizeof(char) * str.size());
      return;
    }
    for (int j = 0; j < upipes.size(); j++) {
      if ((upipes[j].src == cmds[i].upipe_read) && (upipes[j].des == id)) {
        cmds[i].in = upipes[j].fd[PIPE_READ];
        std::string str = "*** " + clients[id].nickname + " (#" +
                          std::to_string(id) + ") just received from " +
                          clients[cmds[i].upipe_read].nickname + " (#" +
                          std::to_string(cmds[i].upipe_read) + ") by \'" +
                          str_cmd + "\' ***\n";
        broadcast(str);
        return;
      }
    }
    cmds[i].dev_null = open("/dev/null", O_RDWR);
    cmds[i].in = cmds[i].dev_null;
    std::string str = "*** Error: the pipe #" +
                      std::to_string(cmds[i].upipe_read) + "->#" +
                      std::to_string(id) + " does not exist yet. ***";
    str.push_back('\n');
    const char *buf = str.c_str();
    write(clients[id].ssock, (const char *)buf, sizeof(char) * str.size());
  } else {
    for (int j = 0; j < clients[id].pipes.size(); j++) {
      if (clients[id].pipes[j].counter == 0) {
        cmds[i].in = clients[id].pipes[j].fd[PIPE_READ];
        return;
      }
    }
  }
  return;
}
void output_check(int id, int i) {
  if (cmds[i].npipe) {
    for (auto &it : clients[id].pipes) {
      if (it.counter == cmds[i].npipe) {
        cmds[i].out = it.fd[PIPE_WRITE];
        return;
      }
    }
    Pipe npipe_temp;
    pipe(npipe_temp.fd);
    cmds[i].out = npipe_temp.fd[PIPE_WRITE];
    npipe_temp.counter = cmds[i].npipe;
    clients[id].pipes.push_back(npipe_temp);
  } else if (cmds[i].lpipe) {
    Pipe lpipe_temp;
    pipe(lpipe_temp.fd);
    cmds[i].out = lpipe_temp.fd[PIPE_WRITE];
    lpipe_temp.counter = 0;
    clients[id].pipes.push_back(lpipe_temp);
  } else if (cmds[i].upipe_write) {
    if (cmds[i].upipe_write < 1 || cmds[i].upipe_write > MAX_CLIENTS ||
        clients[cmds[i].upipe_write].ssock == -1) {
      if (!cmds[i].dev_null) cmds[i].dev_null = open("/dev/null", O_RDWR);
      cmds[i].out = cmds[i].dev_null;
      std::string str = "*** Error: user #" +
                        std::to_string(cmds[i].upipe_write) +
                        " does not exist yet. ***";
      str.push_back('\n');
      const char *buf = str.c_str();
      write(clients[id].ssock, (const char *)buf, sizeof(char) * str.size());
      return;
    }
    for (auto &it : upipes) {
      if ((it.des == cmds[i].upipe_write) && (it.src == id)) {
        if (!cmds[i].dev_null) cmds[i].dev_null = open("/dev/null", O_RDWR);
        cmds[i].out = cmds[i].dev_null;
        std::string str = "*** Error: the pipe #" + std::to_string(id) + "->#" +
                          std::to_string(cmds[i].upipe_write) +
                          " already exists. ***";
        str.push_back('\n');
        const char *buf = str.c_str();
        write(clients[id].ssock, (const char *)buf, sizeof(char) * str.size());
        return;
      }
    }
    Upipe pipe_temp;
    pipe(pipe_temp.fd);
    cmds[i].out = pipe_temp.fd[PIPE_WRITE];
    pipe_temp.src = id;
    pipe_temp.des = cmds[i].upipe_write;
    upipes.push_back(pipe_temp);
    std::string str = "*** " + clients[id].nickname + " (#" +
                      std::to_string(id) + ") just piped \'" + str_cmd +
                      "\' to " + clients[cmds[i].upipe_write].nickname + " (#" +
                      std::to_string(cmds[i].upipe_write) + ") ***\n";
    broadcast(str);
  }
}
void pipes_decrease(int id) {
  for (auto &it : clients[id].pipes) it.counter--;
}
bool cmd_exec(int id) {
  clients[id].print_mod = true;
  setenv("PATH", clients[id].env["PATH"].c_str(), 1);
  bool npipe_end = false;
  for (int i = 0; i < cmds.size(); i++) {
    npipe_end = false;
    if (cmds[i].argv[0] == "printenv") {
      std::string temp = "";
      if (getenv(cmds[i].argv[1].c_str()))
        temp = getenv(cmds[i].argv[1].c_str());
      if (!(temp == "")) {
        temp.push_back('\n');
        const char *buf = temp.c_str();
        write(clients[id].ssock, (const char *)buf, sizeof(char) * temp.size());
      }
    } else if (cmds[i].argv[0] == "setenv") {
      setenv(cmds[i].argv[1].c_str(), cmds[i].argv[2].c_str(), 1);
      clients[id].env[cmds[i].argv[1].c_str()] = cmds[i].argv[2];
    } else if (cmds[i].argv[0] == "who") {
      std::string str = "<ID>\t<nickname>\t<IP:port>\t<indicate me>";
      str.push_back('\n');
      const char *buf = str.c_str();
      write(clients[id].ssock, (const char *)buf, sizeof(char) * str.size());
      for (int j = 1; j < MAX_CLIENTS + 1; j++) {
        if (clients[j].ssock != -1) {
          std::string str1 = std::to_string(j) + "\t" + clients[j].nickname +
                             "\t" + clients[j].ip + ":" + clients[j].port;
          if (id == j) str1 += "\t<-me";
          str1 += '\n';
          const char *buf1 = str1.c_str();
          write(clients[id].ssock, (const char *)buf1,
                sizeof(char) * str1.size());
        }
      }
    } else if (cmds[i].argv[0] == "name") {
      std::string new_name = cmds[i].argv[1];
      for (int j = 1; j < MAX_CLIENTS + 1; j++) {
        if (clients[j].ssock != -1 && clients[j].nickname == new_name) {
          std::string str = "*** User \'" + new_name + "\' already exists. ***";
          str.push_back('\n');
          const char *buf = str.c_str();
          write(clients[id].ssock, (const char *)buf,
                sizeof(char) * str.size());
          return npipe_end;
        }
      }
      clients[id].nickname = new_name;
      std::string str = "*** User from " + clients[id].ip + ":" +
                        clients[id].port + " is named \'" + new_name +
                        "\'. ***\n";
      broadcast(str);
    } else if (cmds[i].argv[0] == "tell") {
      if (clients[stoi(cmds[i].argv[1])].ssock == -1) {
        std::string str =
            "*** Error: user #" + cmds[i].argv[1] + " does not exist yet. ***";
        str += '\n';
        const char *buf = str.c_str();
        write(clients[id].ssock, (const char *)buf, sizeof(char) * str.size());
      } else {
        std::string str = "*** " + clients[id].nickname + " told you ***:";
        for (int j = 2; j < cmds[i].argv.size(); j++) {
          str = str + " " + cmds[i].argv[j];
        }
        str += '\n';
        const char *buf = str.c_str();
        write(clients[stoi(cmds[i].argv[1])].ssock, (const char *)buf,
              sizeof(char) * str.size());
      }
    } else if (cmds[i].argv[0] == "yell") {
      std::string str = "*** " + clients[id].nickname + " yelled ***:";
      for (int j = 1; j < cmds[i].argv.size(); j++) {
        str = str + " " + cmds[i].argv[j];
      }
      str += '\n';
      broadcast(str);
    } else {
      input_check(id, i);
      if (cmds[i].npipe || cmds[i].lpipe || cmds[i].upipe_write)
        output_check(id, i);
      pid_t pid = fork();
      while (pid == -1) pid = fork();
      if (pid == 0) {
        dup2(clients[id].ssock, STDIN_FILENO);
        dup2(clients[id].ssock, STDOUT_FILENO);
        dup2(clients[id].ssock, STDERR_FILENO);
        std::vector<const char *> args;
        std::string file_name = "";
        for (auto &it : cmds[i].argv) args.push_back(it.c_str());
        if (cmds[i].in) dup2(cmds[i].in, STDIN_FILENO);
        if (cmds[i].out || cmds[i].err) {
          if (cmds[i].out) dup2(cmds[i].out, STDOUT_FILENO);
          if (cmds[i].err) dup2(cmds[i].out, STDERR_FILENO);
        }
        if (cmds[i].file_tag) {
          file_name = cmds[i].argv[cmds[i].argv.size() - 1];
          args.pop_back();
          args.pop_back();
          int file1 =
              open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
          dup2(file1, STDOUT_FILENO);
          close(file1);
        }
        for (int j = 0; j < clients[id].pipes.size(); j++) {
          close(clients[id].pipes[j].fd[PIPE_READ]);
          close(clients[id].pipes[j].fd[PIPE_WRITE]);
        }
        for (int j = 0; j < upipes.size(); j++) {
          close(upipes[j].fd[PIPE_READ]);
          close(upipes[j].fd[PIPE_WRITE]);
        }
        if (cmds[i].dev_null) close(cmds[i].dev_null);
        args.push_back(nullptr);
        if (execvp(cmds[i].argv[0].c_str(),
                   const_cast<char *const *>(args.data())) == -1)
          std::cerr << "Unknown command: [" << cmds[i].argv[0] << "]."
                    << std::endl;
        exit(0);
      } else {
        int status;
        if (cmds[i].in) {
          for (int j = 0; j < clients[id].pipes.size(); j++) {
            if (cmds[i].in == clients[id].pipes[j].fd[PIPE_READ] &&
                clients[id].pipes[j].counter == 0) {
              close(clients[id].pipes[j].fd[PIPE_READ]);
              close(clients[id].pipes[j].fd[PIPE_WRITE]);
              clients[id].pipes.erase(clients[id].pipes.begin() + j);
              break;
            }
          }
          for (int j = 0; j < upipes.size(); j++) {
            if (cmds[i].in == upipes[j].fd[PIPE_READ]) {
              close(upipes[j].fd[PIPE_READ]);
              close(upipes[j].fd[PIPE_WRITE]);
              upipes.erase(upipes.begin() + j);
              break;
            }
          }
        }
        if (cmds[i].dev_null) close(cmds[i].dev_null);
        if (cmds[i].npipe) {
          pipes_decrease(id);
          npipe_end = true;
        }
        if (cmds[i].lpipe || cmds[i].npipe) {
          usleep(1000);
          waitpid(-1, &status, WNOHANG);
        } else
          waitpid(pid, &status, 0);
      }
    }
  }
  return npipe_end;
}
void npshell(int id) {
  initial();
  char content[15000];
  bzero((char *)&content, sizeof(content));
  if (read(clients[id].ssock, content, sizeof(content)) < 0) {
    perror("read fail");
    return;
  }
  // std::cout<<id<<":"<<content;
  std::string str = content;
  if (str.back() == '\n') str.pop_back();
  if (str.back() == '\r') str.pop_back();
  str_cmd = str;
  if (str == "exit" || std::cin.eof()) {
    std::string str1 = "*** User \'" + clients[id].nickname + "\' left. ***\n";
    FD_CLR(clients[id].ssock, &afds);
    close(clients[id].ssock);
    for (int j = 0; j < upipes.size(); j++) {
      if (upipes[j].src == id || upipes[j].des == id) {
        close(upipes[j].fd[0]);
        close(upipes[j].fd[1]);
        upipes.erase(upipes.begin() + j);
      }
    }
    clients[id].reset();
    broadcast(str1);
    return;
  } else {
    cmd_slice(str);
    if (!cmd_exec(id)) pipes_decrease(id);
    cmds.clear();
    if (clients[id].print_mod) {
      write(clients[id].ssock, (const void *)mod_message, sizeof(char) * 2);
      clients[id].print_mod = false;
    }
  }
  return;
}
void client_add(int msock) {
  for (int i = 1; i < MAX_CLIENTS + 1; i++) {
    if (clients[i].ssock != -1)
      continue;
    else {
      clients[i].reset();
      int ssock;
      sockaddr_in cli_addr;
      socklen_t cli_len = sizeof(cli_addr);
      if ((ssock = accept(msock, (sockaddr *)&cli_addr, &cli_len))) {
        clients[i].ssock = ssock;
        clients[i].ip = inet_ntoa(cli_addr.sin_addr);
        clients[i].port = std::to_string(htons(cli_addr.sin_port));
        clients[i].env["PATH"] = "bin:.";
        FD_SET(clients[i].ssock, &afds);
        std::string str =
            "****************************************\n** Welcome to the "
            "information server. "
            "**\n****************************************\n";
        const char *buf = str.c_str();
        write(clients[i].ssock, (const void *)buf, sizeof(char) * str.size());
        std::string login_message = "*** User \'" + clients[i].nickname +
                                    "\' entered from " + clients[i].ip + ":" +
                                    clients[i].port + ". ***\n";
        broadcast(login_message);
        write(clients[i].ssock, (const void *)mod_message, sizeof(char) * 2);
        return;
      }
    }
  }
  return;
}
int main(int argc, char *argv[]) {
  unsigned short port = 7001;
  if (argc > 1) port = (unsigned short)atoi(argv[1]);
  int msock, optval = 1;
  sockaddr_in fsin;
  socklen_t alen;
  if ((msock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    std::cerr << "server: can't open stream socket";
  bzero((char *)&fsin, sizeof(fsin));
  fsin.sin_family = AF_INET;
  fsin.sin_addr.s_addr = INADDR_ANY;
  fsin.sin_port = htons(port);
  setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (bind(msock, (sockaddr *)&fsin, sizeof(fsin)) < 0)
    std::cerr << "server: can't bind local address";
  listen(msock, QLEN);
  FD_ZERO(&afds);
  FD_SET(msock, &afds);
  while (true) {
    memcpy(&rfds, &afds, sizeof(rfds));
    nfds = msock;
    for (int i = 1; i < MAX_CLIENTS + 1; i++) {
      if (nfds < clients[i].ssock) {
        nfds = clients[i].ssock;
      }
    }
    if (select(nfds + 1, &rfds, NULL, NULL, NULL) < 0) {
      continue;
    } else {
      if (FD_ISSET(msock, &rfds)) client_add(msock);
      for (int i = 1; i < MAX_CLIENTS + 1; i++)
        if ((clients[i].ssock != -1) && FD_ISSET(clients[i].ssock, &rfds))
          npshell(i);
    }
  }
  return 0;
}