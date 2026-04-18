#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int32_t sock_fd;
  int16_t x, y;
  bool active;
} Player;

void print_players();

// registers a player
// returns player index on success
// returns -1 on failure
int16_t register_player(int fd);

// Moves the player by dx and dy
void move_player_position(int player_index, int16_t x, int16_t y);

void mark_player_inactive(int player_index);

// Streams the player position to all other players except the current player
void stream_player_event(
    int player_index, HeaderMessage* send_header, void* payload, size_t payload_size
);

#endif
