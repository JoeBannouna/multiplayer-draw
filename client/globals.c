#ifndef GLOBAL_C
#define GLOBAL_C

#include <pthread.h>

#include "player.h"
#include "queue.h"

extern Queue actions_queue;
extern pthread_mutex_t actions_queue_mutex;
extern bool connected;
extern char host[];
extern pthread_cond_t actions_queue_cond;

#define MAX_PLAYERS 20
#define BACKLOG MAX_PLAYERS  // how many pending connections queue will hold

extern Player players[];
extern pthread_mutex_t players_mutex;

#endif
