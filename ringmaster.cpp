#include "potato.h"

using namespace std;

void endGame(Potato & potato, vector<Player> & players) {
  for (size_t i = 0; i < players.size(); ++i) {
    if (sizeof(Potato) != send(players[i].fd, &potato, sizeof(Potato), 0)) {
      cerr << "Fail to end the game\n";
      exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char ** argv) {
  if (argc < 4) {
    cerr << "Usage: ./ringmaster <port_num> <num_players> <num_hops>" << endl;
    return EXIT_FAILURE;
  }
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  // Validate command line argument
  // num_players > 1, num_hops [0, 512]
  if (num_players < 2 || num_hops < 0 || num_hops > 512) {
    cerr << "Usage: num_players > 1, num_hops [0, 512].\n";
    return EXIT_FAILURE;
  }

  cout << "Potato Ringmaster\n";
  cout << "Players = " << num_players << endl;
  cout << "Hops = " << num_hops << endl;

  vector<Player> players;

  // Master starts to Listen.
  char * port = argv[1];
  TCPServer * master = new TCPServer();
  int listen_fd = master->startListen(port);
  for (int i = 0; i < num_players; ++i) {
    string player_hostname;
    int player_fd;
    master->acceptConnection(listen_fd, player_fd, player_hostname);
    players.push_back(Player(player_fd, player_hostname));
    recv(players[i].fd, &players[i].port, sizeof(players[i].port), 0);
    send(players[i].fd, &i, sizeof(i), 0);
    send(players[i].fd, &num_players, sizeof(num_players), 0);
    cout << "Player " << i << " is ready to play\n";
  }

 
  // Send neighbors' info to each player.
  for (int i = 0; i < num_players; ++i) {
    int neigh = (i + 1) % num_players;
    if (sizeof(Player) != send(players[i].fd, &players[neigh], sizeof(Player), 0)) {
      cerr << "Fail to send player info\n";
      return EXIT_FAILURE;
    }
  }

  // Create potato object
  // # hops & list of potatos
  Potato potato;
  potato.num_hops = num_hops;
  if (num_hops == 0) {
    endGame(potato, players);
    return EXIT_SUCCESS;
  }
  // Send to a random player
  srand((unsigned int)time(NULL));
  int random = rand() % num_players;
  if (sizeof(Potato) != send(players[random].fd, &potato, sizeof(Potato), 0)) {
    cerr << "Fail to send random potato\n";
    return EXIT_FAILURE;
  }
  cout << "Ready to start the game, sending potato to " << random << endl;

  fd_set rfds;
  FD_ZERO(&rfds);
  for (size_t i = 0; i < players.size(); ++i) {
    FD_SET(players[i].fd, &rfds);
  }
  // Listen 'it' to return.
  int nfd = players[num_players - 1].fd + 1;
  if (select(nfd, &rfds, NULL, NULL, NULL) == -1) {
    cerr << "Select: \n";
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < players.size(); ++i) {
    if (FD_ISSET(players[i].fd, &rfds)) {
      if (sizeof(Potato) != recv(players[i].fd, &potato, sizeof(Potato), MSG_WAITALL)) {
        cerr << "Fail to receive potato from 'it'" << endl;
        return EXIT_FAILURE;
      }
      break;
    }
  }
  endGame(potato, players);
  potato.print_trace();
  delete master;
  sleep(1);
  close(listen_fd);
  for (int i = 0; i < num_players - 1; ++i) {
    close(players[i].fd);
  }
}
