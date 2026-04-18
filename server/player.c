#include "player.h"

#include <stdbool.h>
#include <stdio.h>

#include "../core/types.h"
#include "state.c"

void print_players() {
  printf("PLAYER CONFIGURATION ");

  pthread_mutex_lock(&players_mutex);
  for (size_t i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active) printf("1 ");
    else printf("0 ");
  }
  pthread_mutex_unlock(&players_mutex);

  printf("\n");
}

int16_t register_player(int fd) {
  int16_t player_index = -1;
  pthread_mutex_lock(&players_mutex);
  for (int16_t i = 0; i < MAX_PLAYERS; i++) {
    if (!players[i].active) {
      // make player active
      players[i].sock_fd = fd;
      players[i].active = true;
      // return its index
      player_index = i;
      break;
    }
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

void stream_player_event(
    int player_index, HeaderMessage* send_header_p, void* payload_p, size_t payload_size
) {
  int target_fds[MAX_PLAYERS];
  int target_count = 0;

  // collect all the valid fds
  pthread_mutex_lock(&players_mutex);
  for (int i = 0; i < MAX_PLAYERS; i++) {
    // stream to all active players except current player
    if (players[i].active && i != player_index) {
      target_fds[target_count] = players[i].sock_fd;
      target_count++;
    }
  }
  pthread_mutex_unlock(&players_mutex);

  // stream to the fds
  for (int i = 0; i < target_count; i++) {
    if (send(target_fds[i], send_header_p, sizeof(HeaderMessage), 0) == -1) perror("send");
    if (payload_size != 0) {
      if (send(target_fds[i], payload_p, payload_size, 0) == -1) perror("send");
    }
  }
}
