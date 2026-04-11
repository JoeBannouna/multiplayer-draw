#include "player.h"

#include <stdbool.h>

#include "globals.c"

int join_player(PlayerPositionUpdatePacket packet) {
  int player_index = -1;
  
  pthread_mutex_lock(&players_mutex);
  if (!players[packet.player_index].active) {
    printf("Added player at index %hd\n", packet.player_index);
    players[packet.player_index].x = packet.x;
    players[packet.player_index].y = packet.y;
    players[packet.player_index].active = true;

    player_index = packet.player_index;
  }
  pthread_mutex_unlock(&players_mutex);

  return player_index;
}

void move_player_position(int player_index, int16_t dx, int16_t dy) {
  pthread_mutex_lock(&players_mutex);
  players[player_index].x = dx;
  players[player_index].y = dy;
  pthread_mutex_unlock(&players_mutex);
}

void mark_player_inactive(int player_index) {
  pthread_mutex_lock(&players_mutex);
  // mark spot inactive
  players[player_index].active = false;
  pthread_mutex_unlock(&players_mutex);
}
