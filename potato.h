#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
using namespace std;

// Potato structure
class Potato {
 public:
  int num_hops;
  int idx_hop;
  int trace[512];
  Potato() : idx_hop(0) { memset(trace, 0, sizeof(trace)); }
  void print_trace() {
    cout << "Trace of potato:\n";
    for (int i = 0; i < idx_hop; i++) {
      cout << trace[i];
      if (i != num_hops - 1) {
        cout << ", ";
      }
    }
    cout << "\n";
  }
};

class Player {
 public:
  int fd;
  int port;
  char hostname[INET6_ADDRSTRLEN];
  Player() {
    memset(this->hostname, 0, sizeof(this->hostname));
  }
  Player(int fd, string hostname) : fd(fd) {
    memset(this->hostname, 0, sizeof(this->hostname));
    strcpy(this->hostname, hostname.c_str());
  }
};

class TCPServer {
 private:
  struct addrinfo host_info;
  struct addrinfo * host_info_list;

 public:
  ~TCPServer() { freeaddrinfo(host_info_list); }
  int startListen(const char * port, const char * hostname);
  int acceptConnection(int socket_fd, int & player_fd, string & play\
er_hostname);
};

// get sockaddr, IPv4 or IPv6:
void * get_in_addr(struct sockaddr * sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int TCPServer::acceptConnection(int socket_fd,
                                int & player_fd,
                                string & player_hostname) {
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connection_fd;
  client_connection_fd =
      accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    exit(EXIT_FAILURE);
  }  //if
  /*  char hostname[INET6_ADDRSTRLEN];
  inet_ntop(socket_addr.ss_family,
            get_in_addr((struct sockaddr *)&socket_addr),
            hostname,
            sizeof(hostname));*/
  struct sockaddr_in * temp = (struct sockaddr_in *)&socket_addr;
  player_hostname = inet_ntoa(temp->sin_addr);
  player_fd = client_connection_fd;
  return ntohs(temp->sin_port);
}

int TCPServer::startListen(const char * port = "", const char * hostname = NULL) {
  int status;
  int socket_fd;
  //const char * hostname = NULL;
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  if (strcmp(port, "") == 0) {
    ((struct sockaddr_in *)host_info_list->ai_addr)->sin_port = 0;
  }

  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  cout << "Waiting for connection on port " << port << endl;
  return socket_fd;
}
