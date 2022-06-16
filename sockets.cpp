#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <iostream>
#include <netdb.h>
#define MALLOC_ERROR "memory allocation failed"
#define TYPE_IDX 1
#define PORT_IDX 2
#define COMMAND_IDX 3
#define BUFFER_SIZE 256
using std::string;
#define MAXHOSTNAME 1024
#define SYS_CALL_ERROR "system call failed"


void exit(const string& error, char* ptr) {
  std::cerr << "system error: " << error << std::endl;
  free(ptr);
  ptr = nullptr;
  exit(EXIT_FAILURE);
}

int write_data(int s, char *buf, int n) {
  ssize_t total_count = 0;       /* counts bytes read */
  ssize_t cur_count = 0;               /* bytes read this pass */

  while (total_count < n) { /* loop until full buffer */
    cur_count = write(s, buf, n-total_count);
    if (cur_count > 0)  {
      total_count += cur_count;
      buf += cur_count;
    }
    if (cur_count < 1) {
      return -1;
    }
  }
  return (int)total_count;
}


void client(int port, char* command, char* ptr) {
  char host_name[MAXHOSTNAME+1];
  struct sockaddr_in address;
  struct hostent *host_ptr;
  int socket_fd;
  gethostname(host_name, MAXHOSTNAME);
  if ((host_ptr= gethostbyname (host_name)) == nullptr) {
    exit(SYS_CALL_ERROR, ptr);
  }

  memset(&address, 0, sizeof(address));
  memcpy((char *)&address.sin_addr , host_ptr->h_addr ,
         host_ptr->h_length);
  address.sin_family = host_ptr->h_addrtype;
  address.sin_port = htons((u_short)port);

  if ((socket_fd = socket(host_ptr->h_addrtype,
                          SOCK_STREAM, 0)) < 0) {
    exit(SYS_CALL_ERROR, ptr);
  }

  if (connect(socket_fd, (struct sockaddr *)&address , sizeof(address)) < 0) {
    close(socket_fd);
    exit(SYS_CALL_ERROR, ptr);;
  }

  if (write_data(socket_fd, command, BUFFER_SIZE) == -1) {
    close(socket_fd);
    exit(SYS_CALL_ERROR, ptr);
  }
  close(socket_fd);
}

int get_connection(int socket_fd) {
  int socket;

  if ((socket = accept(socket_fd, nullptr, nullptr)) < 0)
    return -1;
  return socket;
}

int read_data(int s, char *buf, int n) {
  int total_count = 0;
  int cur_count = 0;

  while (total_count < n) {
    cur_count = read(s, buf, n-total_count);
    if (cur_count > 0)  {
      total_count += cur_count;
      buf += cur_count;
    }
    if (cur_count < 1) {
      return -1;
    }
  }
  return(total_count);
}



void server(int port) {
  char host_name[MAXHOSTNAME+1];
  int socket_fd, new_socket_fd;
  struct sockaddr_in address;
  struct hostent *host_ptr;

  if (gethostname(host_name, MAXHOSTNAME) == -1) {
    exit(SYS_CALL_ERROR, nullptr);
  }
  host_ptr = gethostbyname(host_name);
  if (host_ptr == nullptr){
    exit(SYS_CALL_ERROR, nullptr);
    }

  memset(&address, 0, sizeof(struct sockaddr_in));
  address.sin_family = host_ptr->h_addrtype;
  /* this is our host address */
  memcpy(&address.sin_addr, host_ptr->h_addr, host_ptr->h_length);
  /* this is our port number */
  address.sin_port= htons(port);

  if ((socket_fd= socket(AF_INET, SOCK_STREAM, 0)) < 0){
    exit(SYS_CALL_ERROR, nullptr);
    }

  if (bind(socket_fd , (struct sockaddr *)&address , sizeof(struct sockaddr_in)) < 0) {
    close(socket_fd);
    exit(SYS_CALL_ERROR, nullptr);
  }

  if (listen(socket_fd, 5) == -1) {
    close(socket_fd);
    exit(SYS_CALL_ERROR, nullptr);
  }

  while(true){
    char buffer[BUFFER_SIZE];
    new_socket_fd = get_connection(socket_fd);
    read_data(new_socket_fd, buffer, BUFFER_SIZE);

    if (system(buffer) == -1) {
      close(new_socket_fd);
      exit(SYS_CALL_ERROR, nullptr);
      }
      close(new_socket_fd);
    }
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    exit(EXIT_FAILURE);
  }
  int port = std::stoi(argv[PORT_IDX]);
  char* final_command;
  if (strcmp(argv[TYPE_IDX],"client") == 0 && argc >= 4) {
    string command = argv[COMMAND_IDX];
    for (int i=COMMAND_IDX+1; i < argc; i++) {
      command += " ";
      command += argv[i];
    }
    final_command = (char*) malloc(command.size()+1);
    if (final_command == nullptr) {
      exit(MALLOC_ERROR, nullptr);
    }
    memcpy(final_command, command.c_str(), command.size());
    client(port, final_command, final_command);
    free(final_command);
  }
  else if (strcmp(argv[TYPE_IDX], "server") == 0) {
    server(port);
  }
  else {
    exit(EXIT_FAILURE);
  }
  return 0;
}
