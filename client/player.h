#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/types.h"

typedef struct {
  int16_t x, y;
  bool active;
} Player;

// registers a player 
// returns player index on success
// returns -1 on failure
int join_player(PlayerPositionUpdatePacket packet);

// Moves the player by dx and dy
void move_player_position(int player_index, int16_t x, int16_t y);

void mark_player_inactive(int player_index);

// Streams the player position to all other players except the current player
void stream_player_position(int player_index);

#endif
