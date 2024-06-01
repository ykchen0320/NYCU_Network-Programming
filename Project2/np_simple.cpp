#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#define PIPE_READ 0
#define PIPE_WRITE 1
#define QLEN 1
class Cmd {
 public:
  std::vector<std::string> argv;
  bool file_tag = false, lpipe = false;
  int npipe = 0, in = 0, out = 0, err = 0;
  void reset() {
    argv.clear();
    file_tag = false;
    lpipe = false;
    npipe = 0;
    in = 0;
    out = 0;
    err = 0;
  }
};
class Pipe {
 public:
  int fd[2] = {0, 0};
  int counter = 0;
};
std::vector<Cmd> cmds;
std::vector<Pipe> pipes;
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
    if (temp[i][0] == '|' || temp[i][0] == '!' || temp[i][0] == '>') {
      if (temp[i] == ">") {
        cmd_temp.file_tag = true;
      } else {
        if (temp[i][0] == '!') cmd_temp.err = 1;
        if (temp[i].size() > 1) {
          temp[i].erase(0, 1);
          cmd_temp.npipe = stoi(temp[i]);
        } else {
          cmd_temp.lpipe = true;
        }
        cmd_temp.argv.pop_back();
        cmds.push_back(cmd_temp);
        cmd_temp.reset();
      }
    }
  }
  if (cmd_temp.argv.size()) cmds.push_back(cmd_temp);
}
void output_check(int i) {
  if (cmds[i].npipe) {
    bool boo = false;
    for (auto &it : pipes) {
      if (it.counter == cmds[i].npipe) {
        cmds[i].out = it.fd[PIPE_WRITE];
        boo = true;
      }
    }
    if (!boo) {
      Pipe npipe_temp;
      pipe(npipe_temp.fd);
      cmds[i].out = npipe_temp.fd[PIPE_WRITE];
      npipe_temp.counter = cmds[i].npipe;
      pipes.push_back(npipe_temp);
    }
  }
  if (cmds[i].lpipe) {
    if (cmds[i].lpipe) {
      Pipe lpipe_temp;
      pipe(lpipe_temp.fd);
      cmds[i].out = lpipe_temp.fd[PIPE_WRITE];
      lpipe_temp.counter = 0;
      pipes.push_back(lpipe_temp);
    }
  }
}
void input_check(int i) {
  for (int j = 0; j < pipes.size(); j++) {
    if (pipes[j].counter == 0) {
      cmds[i].in = pipes[j].fd[PIPE_READ];
      break;
    }
  }
}
void pipes_decrease() {
  for (auto &it : pipes) it.counter--;
}
bool cmd_exec() {
  bool npipe_end = false;
  for (int i = 0; i < cmds.size(); i++) {
    npipe_end = false;
    if (cmds[i].argv[0] == "printenv") {
      std::string temp = "";
      if (getenv(cmds[i].argv[1].c_str()))
        temp = getenv(cmds[i].argv[1].c_str());
      if (!(temp == "")) std::cout << temp << std::endl;
    } else if (cmds[i].argv[0] == "setenv") {
      setenv(cmds[i].argv[1].c_str(), cmds[i].argv[2].c_str(), 1);
    } else {
      input_check(i);
      if (cmds[i].npipe || cmds[i].lpipe) output_check(i);
      pid_t pid = fork();
      while (pid == -1) pid = fork();
      if (pid == 0) {
        std::vector<const char *> args;
        std::string file_name = "";
        for (auto &it : cmds[i].argv) args.push_back(it.c_str());
        if (cmds[i].in) {
          dup2(cmds[i].in, STDIN_FILENO);
        }
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
        for (int j = 0; j < pipes.size(); j++) {
          close(pipes[j].fd[PIPE_READ]);
          close(pipes[j].fd[PIPE_WRITE]);
        }
        args.push_back(nullptr);
        if (execvp(cmds[i].argv[0].c_str(),
                   const_cast<char *const *>(args.data())) == -1)
          std::cerr << "Unknown command: [" << cmds[i].argv[0] << "]."
                    << std::endl;
        exit(0);
      } else {
        int status;
        if (cmds[i].in) {
          for (int j = 0; j < pipes.size(); j++) {
            if (cmds[i].in == pipes[j].fd[PIPE_READ] && pipes[j].counter == 0) {
              close(pipes[j].fd[PIPE_READ]);
              close(pipes[j].fd[PIPE_WRITE]);
              pipes.erase(pipes.begin() + j);
              break;
            }
          }
        }
        if (cmds[i].npipe) {
          pipes_decrease();
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
int npshell() {
  initial();
  std::string str;
  while (true) {
    std::cout << "% ";
    getline(std::cin, str);
    if (str[str.size() - 1] == '\r') str.pop_back();
    if (str == "exit" || std::cin.eof()) exit(0);
    if (str == "") continue;
    cmd_slice(str);
    if (!cmd_exec()) pipes_decrease();
    cmds.clear();
  }
  return 0;
}
int main(int argc, char *argv[]) {
  unsigned short port = 7001;
  if (argc > 1) port = (unsigned short)atoi(argv[1]);
  int sockfd, newsockfd, childpid, optval = 1;
  socklen_t clilen;
  sockaddr_in cli_addr, serv_addr;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    std::cerr << "server: can't open stream socket";
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (bind(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    std::cerr << "server: can't bind local address";
  listen(sockfd, QLEN);
  while (true) {
    clilen = sizeof(cli_addr);
    if ((newsockfd = accept(sockfd, (sockaddr *)&cli_addr, &clilen)) < 0)
      std::cerr << "server: accept error";
    if ((childpid = fork()) < 0)
      std::cerr << "server: fork error";
    else if (childpid == 0) {
      close(sockfd);
      dup2(newsockfd, STDIN_FILENO);
      dup2(newsockfd, STDOUT_FILENO);
      dup2(newsockfd, STDERR_FILENO);
      npshell();
    }
    close(newsockfd);
  }
  return 0;
}