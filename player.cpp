#include "potato.h"

using namespace std;

int connectMaster(const char * hostname, const char * port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

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

  cout << "Connecting to " << hostname << " on port " << port << "..." << endl;

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  freeaddrinfo(host_info_list);
  return socket_fd;
}

// Validate command line argument
// num_players >= 1, num_hops [0, 512]
int main(int argc, char ** argv) {
  if (argc < 3) {
    cerr << "Usage: player <machine_name> <port_num>" << endl;
  }
  const char * master_host_name = argv[1];
  const char * master_port = argv[2];

  int master_fd = connectMaster(master_host_name, master_port);
  int num_players;
  int player_id;
  TCPServer * master = new TCPServer();
  int left_fd = master->startListen();
  struct sockaddr_in sa;
  socklen_t sa_len = sizeof(sa);
  if (getsockname(left_fd, (struct sockaddr *)&sa, &sa_len) == -1) {
    perror("getsockname() failed");
    return EXIT_FAILURE;
  }
  int left_port = ntohs(sa.sin_port);
  send(master_fd, &left_port, sizeof(left_port), 0);
  recv(master_fd, &player_id, sizeof(player_id), 0);
  recv(master_fd, &num_players, sizeof(num_players), 0);
  cout << "Connected as player " << player_id << " out of " << num_players
       << " total players\n";

  Player right_neighbor;
  int temp = 0;
  if (sizeof(Player) !=
      (temp = recv(master_fd, &right_neighbor, sizeof(Player), MSG_WAITALL))) {
    cout << "Receive size -" << temp << endl;
    cout << "Receive incomplete neighbor info.\n";
    return EXIT_FAILURE;
  }

  //Connect right neighbor
  char right_neighbor_port[10];
  sprintf(right_neighbor_port, "%d", right_neighbor.port);
  right_neighbor.fd = connectMaster(right_neighbor.hostname, right_neighbor_port);
  cout << "Connect to Right - Neighbor : fd :" << right_neighbor.fd <<" Hostname - " << right_neighbor.hostname
       << " Port - " << right_neighbor_port << endl;
  //Accept left neighbor
  Player left_neighbor;
  string left_hostname;
  left_neighbor.port = master->acceptConnection(left_fd, left_neighbor.fd, left_hostname);
  strcpy(left_neighbor.hostname, left_hostname.c_str());
  cout << "Accept: Left- Neighbor : Hostname - " << left_neighbor.hostname << " Port - "
       << left_neighbor.port << endl;

  int fds[] = {left_neighbor.fd, right_neighbor.fd, master_fd};

  while (1) {
    fd_set rfds;
    FD_ZERO(&rfds);
    for (size_t i = 0; i < 3; ++i) {
      FD_SET(fds[i], &rfds);
    }
    // Listen 'it' to return.
    int nfd =
        (right_neighbor.fd > left_neighbor.fd ? right_neighbor.fd : left_neighbor.fd) + 1;
    if (select(nfd, &rfds, NULL, NULL, NULL) == -1) {
      cerr << "Select: \n";
      return EXIT_FAILURE;
    }
    Potato potato;
    srand((unsigned int)time(NULL) + player_id);
    for (size_t i = 0; i < 3; ++i) {
      if (FD_ISSET(fds[i], &rfds)) {
        // Receives the potato
        if (sizeof(Potato) != recv(fds[i], &potato, sizeof(Potato), MSG_WAITALL)) {
          cerr << "Fail to receive potato" << endl;
          return EXIT_FAILURE;
        }
        break;
      }
    }

    if (potato.num_hops == 0) {
      // End game.
      break;
    }
    else {
      // # hops --
      potato.num_hops--;
      potato.trace[potato.idx_hop++] = player_id;
      if (potato.num_hops == 0) {
        if (sizeof(Potato) != send(master_fd, &potato, sizeof(Potato), 0)) {
          cerr << "Fail to send potato back to ringmaster\n";
          break;
        }
        cout << "I’ it\n";
      }
      else {
        int next = rand() % 2;
        int next_idx = (next == 1) ? (player_id + 1) % num_players
                                   : (player_id - 1 + num_players) % num_players;
        cout << "Sending potato to " << next_idx << endl;
        if (sizeof(Potato) != send(fds[next_idx], &potato, sizeof(Potato), 0)) {
          cerr << "Fail to send potato back to next player\n";
          break;
        }
      }
    }
  }

  sleep(1);
  for (size_t i = 0; i < 3; ++i) {
    close(fds[i]);
  }
  return EXIT_SUCCESS;
}
