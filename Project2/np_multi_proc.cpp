#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#define PIPE_READ 0
#define PIPE_WRITE 1
#define QLEN 1
#define MAX_CLIENTS 30
#define SHMKEY1 ((key_t)7890)
#define SHMKEY2 ((key_t)7891)
#define PERMS 0666
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
class Client {
 public:
  unsigned int pid = 0, port = 0;
  char ip[30];
  char nickname[50];
  void reset() {
    pid = 0;
    port = 0;
    strcpy(ip, "");
    strcpy(nickname, "(no name)");
  }
};
class ShareMessage {
 public:
  char message[1024];
};
int shmid_client, shmid_message, clisem, servsem;
std::string str_cmd, fifoPath = "./user_pipe/";
Client *clients;
ShareMessage *message;
std::vector<Cmd> cmds;
std::vector<Pipe> pipes;
std::mutex client_mutex, message_mutex, mail_mutex;
const char *mod_message = "% ";
int mailBox[MAX_CLIENTS + 1] = {0};
void pidcheck() {
  for (int i = 1; i < MAX_CLIENTS + 1; i++) {
    if (clients[i].pid) {
      std::cout << i << ": " << clients[i].pid << std::endl;
    }
  }
}
static void signal_handler(int sig) {
  if (sig == SIGCHLD) {
    int wait_temp;
    while (waitpid(-1, &wait_temp, WNOHANG) > 0) {
    }
  } else if (sig == SIGINT) {
    shmdt(clients);
    shmdt(message);
    shmctl(shmid_client, IPC_RMID, NULL);
    shmctl(shmid_message, IPC_RMID, NULL);
    exit(0);
  } else if (sig == SIGUSR1) {
    std::cout << message->message << std::endl;
  }
}
void user_pipe_handler(int signum, siginfo_t *info, void *context) {
  int senderPid = info->si_pid, receiverPid = getpid(), senderID = 0,
      receiverID = 0;
  while (senderID == 0 || receiverID == 0) {
    for (int i = 1; i < MAX_CLIENTS + 1; i++) {
      if (clients[i].pid == senderPid) senderID = i;
      if (clients[i].pid == receiverPid) receiverID = i;
    }
  }
  std::string temp =
      fifoPath + std::to_string(senderID) + "_" + std::to_string(receiverID);
  const char *fifoFile = temp.c_str();
  mailBox[senderID] = open(fifoFile, O_RDONLY);
}
void initial() {
  signal(SIGCHLD, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGUSR1, signal_handler);
  struct sigaction sa;
  sa.sa_sigaction = user_pipe_handler;
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGUSR2, &sa, NULL);
  setenv("PATH", "bin:.", 1);
  return;
}
void broadcast(std::string str) {
  const char *buf = str.c_str();
  message_mutex.lock();
  strcpy(message->message, buf);
  for (int i = 1; i < MAX_CLIENTS + 1; i++)
    if (clients[i].port) kill(clients[i].pid, SIGUSR1);
  usleep(1000);
  message_mutex.unlock();
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
        !clients[cmds[i].upipe_read].port) {
      cmds[i].dev_null = open("/dev/null", O_RDWR);
      cmds[i].in = cmds[i].dev_null;
      std::cout << "*** Error: user #" << std::to_string(cmds[i].upipe_read)
                << " does not exist yet. ***" << std::endl;
      return;
    }
    if (mailBox[cmds[i].upipe_read]) {
      std::string str = "*** " + std::string(clients[id].nickname) + " (#" +
                        std::to_string(id) + ") just received from " +
                        clients[cmds[i].upipe_read].nickname + " (#" +
                        std::to_string(cmds[i].upipe_read) + ") by \'" +
                        str_cmd + "\' ***";
      broadcast(str);
      cmds[i].in = mailBox[cmds[i].upipe_read];
      return;
    } else {
      cmds[i].dev_null = open("/dev/null", O_RDWR);
      cmds[i].in = cmds[i].dev_null;
      std::cout << "*** Error: the pipe #"
                << std::to_string(cmds[i].upipe_read) + "->#"
                << std::to_string(id) << " does not exist yet. ***"
                << std::endl;
    }
  } else {
    for (int j = 0; j < pipes.size(); j++) {
      if (pipes[j].counter == 0) {
        cmds[i].in = pipes[j].fd[PIPE_READ];
        return;
      }
    }
  }
  return;
}
void output_check(int id, int i) {
  if (cmds[i].npipe) {
    for (auto &it : pipes) {
      if (it.counter == cmds[i].npipe) {
        cmds[i].out = it.fd[PIPE_WRITE];
        return;
      }
    }
    Pipe npipe_temp;
    pipe(npipe_temp.fd);
    cmds[i].out = npipe_temp.fd[PIPE_WRITE];
    npipe_temp.counter = cmds[i].npipe;
    pipes.push_back(npipe_temp);
  } else if (cmds[i].lpipe) {
    Pipe lpipe_temp;
    pipe(lpipe_temp.fd);
    cmds[i].out = lpipe_temp.fd[PIPE_WRITE];
    lpipe_temp.counter = 0;
    pipes.push_back(lpipe_temp);
  } else if (cmds[i].upipe_write) {
    if (cmds[i].upipe_write < 1 || cmds[i].upipe_write > MAX_CLIENTS ||
        !clients[cmds[i].upipe_write].port) {
      if (!cmds[i].dev_null) cmds[i].dev_null = open("/dev/null", O_RDWR);
      cmds[i].out = cmds[i].dev_null;
      std::cout << "*** Error: user #" << std::to_string(cmds[i].upipe_write)
                << " does not exist yet. ***" << std::endl;
      return;
    }
    std::string temp = fifoPath + std::to_string(id) + "_" +
                       std::to_string(cmds[i].upipe_write);
    const char *fifoFile = temp.c_str();
    if (access(fifoFile, F_OK) == 0) {
      if (!cmds[i].dev_null) cmds[i].dev_null = open("/dev/null", O_RDWR);
      cmds[i].out = cmds[i].dev_null;
      std::cout << "*** Error: the pipe #" << std::to_string(id) << "->#"
                << std::to_string(cmds[i].upipe_write) << " already exists. ***"
                << std::endl;
      return;
    } else {
      std::string str = "*** " + std::string(clients[id].nickname) + " (#" +
                        std::to_string(id) + ") just piped \'" + str_cmd +
                        "\' to " + clients[cmds[i].upipe_write].nickname +
                        " (#" + std::to_string(cmds[i].upipe_write) + ") ***";
      broadcast(str);
      mknod(fifoFile, S_IFIFO | PERMS, 0);
      kill(clients[cmds[i].upipe_write].pid, SIGUSR2);
      cmds[i].out = open(fifoFile, O_RDWR);
    }
  }
}
void pipes_decrease() {
  for (auto &it : pipes) it.counter--;
}
bool cmd_exec(int id) {
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
    } else if (cmds[i].argv[0] == "who") {
      std::cout << "<ID>\t<nickname>\t<IP:port>\t<indicate me>" << std::endl;
      for (int j = 1; j < MAX_CLIENTS + 1; j++) {
        if (clients[j].port) {
          std::string str1 = std::to_string(j) + "\t" + clients[j].nickname +
                             "\t" + clients[j].ip + ":" +
                             std::to_string(clients[j].port);
          if (j == id) str1 += "\t<-me";
          std::cout << str1 << std::endl;
        }
      }
    } else if (cmds[i].argv[0] == "name") {
      std::string new_name = cmds[i].argv[1];
      for (int j = 1; j < MAX_CLIENTS + 1; j++) {
        if (clients[j].port && clients[j].nickname == new_name) {
          std::cout << "*** User \'" << new_name << "\' already exists. ***"
                    << std::endl;
          return npipe_end;
        }
      }
      client_mutex.lock();
      strcpy(clients[id].nickname, new_name.c_str());
      client_mutex.unlock();
      std::string str = "*** User from " + std::string(clients[id].ip) + ":" +
                        std::to_string(clients[id].port) + " is named \'" +
                        new_name + "\'. ***";
      broadcast(str);
    } else if (cmds[i].argv[0] == "tell") {
      if (!clients[stoi(cmds[i].argv[1])].port) {
        std::cout << "*** Error: user #" << cmds[i].argv[1]
                  << " does not exist yet. ***" << std::endl;
      } else {
        std::string str =
            "*** " + std::string(clients[id].nickname) + " told you ***:";
        for (int j = 2; j < cmds[i].argv.size(); j++) {
          str = str + " " + cmds[i].argv[j];
        }
        const char *buf = str.c_str();
        message_mutex.lock();
        strcpy(message->message, buf);
        message_mutex.unlock();
        kill(clients[stoi(cmds[i].argv[1])].pid, SIGUSR1);
      }
    } else if (cmds[i].argv[0] == "yell") {
      std::string str =
          "*** " + std::string(clients[id].nickname) + " yelled ***:";
      for (int j = 1; j < cmds[i].argv.size(); j++) {
        str = str + " " + cmds[i].argv[j];
      }
      broadcast(str);
    } else {
      input_check(id, i);
      if (cmds[i].npipe || cmds[i].lpipe || cmds[i].upipe_write)
        output_check(id, i);
      pid_t pid = fork();
      while (pid == -1) pid = fork();
      if (pid == 0) {
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
        for (int j = 0; j < pipes.size(); j++) {
          close(pipes[j].fd[PIPE_READ]);
          close(pipes[j].fd[PIPE_WRITE]);
        }
        if (cmds[i].upipe_read) close(cmds[i].in);
        if (cmds[i].upipe_write) close(cmds[i].out);
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
          for (int j = 0; j < pipes.size(); j++) {
            if (cmds[i].in == pipes[j].fd[PIPE_READ] && pipes[j].counter == 0) {
              close(pipes[j].fd[PIPE_READ]);
              close(pipes[j].fd[PIPE_WRITE]);
              pipes.erase(pipes.begin() + j);
              break;
            }
          }
          if (cmds[i].upipe_read) {
            std::string temp = fifoPath + std::to_string(cmds[i].upipe_read) +
                               "_" + std::to_string(id);
            const char *fifoFile = temp.c_str();
            close(cmds[i].in);
            mailBox[cmds[i].upipe_read] = 0;
            unlink(fifoFile);
          }
        }
        if (cmds[i].upipe_write) close(cmds[i].out);
        if (cmds[i].dev_null) close(cmds[i].dev_null);
        if (cmds[i].npipe) {
          pipes_decrease();
          npipe_end = true;
        }
        usleep(1000);
        if (cmds[i].lpipe || cmds[i].npipe || cmds[i].upipe_read) {
          usleep(1000);
          waitpid(-1, &status, WNOHANG);
        } else {
          waitpid(pid, &status, 0);
        }
      }
    }
  }
  return npipe_end;
}
int npshell(int id) {
  std::string welcome_message =
      "****************************************\n** Welcome to the "
      "information server. "
      "**\n****************************************";
  std::cout << welcome_message << std::endl;
  std::string login_message =
      "*** User \'" + std::string(clients[id].nickname) + "\' entered from " +
      clients[id].ip + ":" + std::to_string(clients[id].port) + ". ***";
  broadcast(login_message);
  while (true) {
    std::cout << mod_message;
    std::string cmdLine;
    while (!getline(std::cin, cmdLine)) std::cin.clear();
    if (cmdLine.back() == '\n') cmdLine.pop_back();
    if (cmdLine.back() == '\r') cmdLine.pop_back();
    str_cmd = cmdLine;
    if (cmdLine == "exit") {
      std::string str1 =
          "*** User \'" + std::string(clients[id].nickname) + "\' left. ***";
      client_mutex.lock();
      clients[id].reset();
      for (int i = 1; i < MAX_CLIENTS + 1; i++) {
        std::string temp =
            fifoPath + std::to_string(id) + "_" + std::to_string(i);
        const char *fifoFile = temp.c_str();
        if (access(fifoFile, F_OK) == 0) unlink(fifoFile);
      }
      for (int i = 1; i < MAX_CLIENTS + 1; i++) {
        std::string temp =
            fifoPath + std::to_string(i) + "_" + std::to_string(id);
        const char *fifoFile = temp.c_str();
        if (access(fifoFile, F_OK) == 0) unlink(fifoFile);
      }
      client_mutex.unlock();
      broadcast(str1);
      exit(0);
    }
    if (cmdLine == "") continue;
    cmd_slice(cmdLine);
    if (!cmd_exec(id)) pipes_decrease();
    cmds.clear();
  }
  return 0;
}
int main(int argc, char *argv[]) {
  initial();
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
  if ((shmid_client = shmget(SHMKEY1, sizeof(Client) * (MAX_CLIENTS + 1),
                             PERMS | IPC_CREAT)) < 0) {
    std::cerr << "server: can't get clients shared memory";
  }
  clients = (Client *)shmat(shmid_client, NULL, 0);
  if ((shmid_message =
           shmget(SHMKEY2, sizeof(char) * 1024, PERMS | IPC_CREAT)) < 0) {
    std::cerr << "server: can't get message shared memory";
  }
  message = (ShareMessage *)shmat(shmid_message, NULL, 0);
  for (int i = 0; i < MAX_CLIENTS + 1; i++) {
    clients[i].reset();
    for (int j = 1; j < MAX_CLIENTS + 1; j++) {
      if (i) {
        std::string temp =
            fifoPath + std::to_string(i) + "_" + std::to_string(j);
        const char *fifoFile = temp.c_str();
        if (access(fifoFile, F_OK) == 0) {
          unlink(fifoFile);
        }
      }
    }
  }
  while (true) {
    clilen = sizeof(cli_addr);
    if ((newsockfd = accept(sockfd, (sockaddr *)&cli_addr, &clilen)) < 0)
      std::cerr << "server: accept error";
    int id;
    for (id = 1; id < MAX_CLIENTS + 1; id++) {
      if (!clients[id].port) {
        break;
      }
    }
    if ((childpid = fork()) < 0)
      std::cerr << "server: fork error";
    else if (childpid == 0) {
      client_mutex.lock();
      clients[id].pid = getpid();
      inet_ntop(AF_INET, &(cli_addr.sin_addr), clients[id].ip, INET_ADDRSTRLEN);
      clients[id].port = ntohs(cli_addr.sin_port);
      client_mutex.unlock();
      close(sockfd);
      dup2(newsockfd, STDIN_FILENO);
      dup2(newsockfd, STDOUT_FILENO);
      dup2(newsockfd, STDERR_FILENO);
      close(newsockfd);
      npshell(id);
    } else {
      close(newsockfd);
    }
  }
  return 0;
}