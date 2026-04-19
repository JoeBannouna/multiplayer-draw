#ifndef GLOBAL_C
#define GLOBAL_C

#include <pthread.h>

#include "player.h"
#include "queue.h"

extern Queue actions_queue;
extern bool connected;
extern char host[];
extern uint16_t my_player_index;

#define MAX_PLAYERS 20
#define BACKLOG MAX_PLAYERS  // how many pending connections queue will hold

extern Player players[];
extern pthread_mutex_t players_mutex;
extern char** global_argv;

#endif
