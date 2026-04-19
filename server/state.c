#include <pthread.h>

#include "player.h"
#include "server_data.h"

#define PORT "3490"  // the port users will be connecting to

#define MAX_PLAYERS 20
#define BACKLOG MAX_PLAYERS  // how many pending connections queue will hold

extern Player players[];
extern pthread_mutex_t players_mutex;
extern ServerData server_data;
extern char** global_argv;
