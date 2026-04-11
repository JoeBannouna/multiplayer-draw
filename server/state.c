#include <pthread.h>

#include "player.h"

#define PORT "3490"  // the port users will be connecting to

#define MAX_PLAYERS 20
#define BACKLOG MAX_PLAYERS  // how many pending connections queue will hold

Player players[];
pthread_mutex_t players_mutex;
