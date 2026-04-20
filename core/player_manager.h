
#ifndef STROKE_MANAGER_H
#define STROKE_MANAGER_H

#include <pthread.h>
#include <stdbool.h>

// the single source of truth for current players connected to the server
typedef struct {
  pthread_mutex_t mutex;
} PlayersManager;

// returns the player index
size_t add_player(const char name);

// adds to the list of functions that are executed on player leaves
// returns a callback id
size_t add_callback_on_player_join(void (*callback_func_ptr)(void));

// adds to the list of functions that are executed on player joins
// returns a callback id
void add_callback_on_player_leave(void (*callback_func_ptr)(void));

// removes a callback with a given id
bool remove_callback_on_player_join(size_t callback_id);

// removes a callback with a given id
void remove_callback_on_player_leave(size_t callback_id);

#endif
