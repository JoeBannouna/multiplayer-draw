#include <netdb.h>
#include <stdlib.h>

#include "../core/types.h"
#include "player.h"

// global variable definitions
#include "state.c"

void* handle_client(void* new_fd_p) {
  int new_fd = *((int*)new_fd_p);
  free((int*)new_fd_p);

  printf("NEW CLIENT BABY\n");

  // register player
  int16_t player_index = register_player(new_fd);

  if (player_index != -1) printf("There is space for your connection!\n");
  else {
    printf("There is NO space for your connection!\n");

    close(new_fd);
    return NULL;
  }

  print_players();

  // stream the new player join to other players
  HeaderMessage header_message = {
      .length = htons(sizeof(PlayerPositionUpdatePacket)), .type = htons(PLAYER_JOIN_EVENT)
  };

  pthread_mutex_lock(&players_mutex);
  printf("Streaming join, Sending %hd and %hd\n", players[player_index].x, players[player_index].y);

  PlayerPositionUpdatePacket current_player_initial_position = {
      .player_index = (int16_t)htons((uint16_t)player_index),
      .x = (int16_t)htons((uint16_t)players[player_index].x),
      .y = (int16_t)htons((uint16_t)players[player_index].y)
  };

  PlayerPositionUpdatePacket placeholder_player_position;

  for (int i = 0; i < MAX_PLAYERS; i++) {
    // stream to all active players except current player
    if (players[i].active && i != player_index) {
      // notifying the current player that player[i] is already connected
      placeholder_player_position = (PlayerPositionUpdatePacket){
          .player_index = (int16_t)htons((uint16_t)i),
          .x = (int16_t)htons((uint16_t)players[i].x),
          .y = (int16_t)htons((uint16_t)players[i].y)
      };
      if (send(players[player_index].sock_fd, &header_message, sizeof(HeaderMessage), 0) == -1)
        perror("send");
      if (send(
              players[player_index].sock_fd, &placeholder_player_position,
              sizeof(PlayerPositionUpdatePacket), 0
          ) == -1)
        perror("send");

      // notifying player[i] that the current player has joined
      if (send(players[i].sock_fd, &header_message, sizeof(HeaderMessage), 0) == -1) perror("send");
      if (send(
              players[i].sock_fd, &current_player_initial_position,
              sizeof(PlayerPositionUpdatePacket), 0
          ) == -1)
        perror("send");
    }
  }
  pthread_mutex_unlock(&players_mutex);

  // stream all existing players in the lobby to this player
  // pthread_mutex_lock(&players_mutex);

  // PlayerPositionUpdatePacket initial_position = {
  //     .player_index = (int16_t)htons((uint16_t)player_index),
  //     .x = (int16_t)htons((uint16_t)players[player_index].x),
  //     .y = (int16_t)htons((uint16_t)players[player_index].y)
  // };

  // for (int i = 0; i < MAX_PLAYERS; i++) {
  //   // stream to all active players except current player
  //   if (players[i].active && i != player_index) {
  //     // printf("Sending to %d\n", player_index);
  //     if (send(players[i].sock_fd, &header_message, sizeof(HeaderMessage), 0) == -1)
  //     perror("send");

  //     if (send(players[i].sock_fd, &initial_position, sizeof(PlayerPositionUpdatePacket), 0) ==
  //     -1)
  //       perror("send");
  //   }
  // }
  // pthread_mutex_unlock(&players_mutex);

  HeaderMessage header;

  ssize_t bytes = 0;

  while (true) {
    bytes = recv(new_fd, &header, sizeof(HeaderMessage), MSG_WAITALL);

    if (bytes == -1) {
      perror("client recv");
      continue;
    }

    if (bytes == 0) {
      printf("Goodbye, %d\n", new_fd);

      mark_player_inactive(player_index);
      close(new_fd);

      return NULL;
    }

    if (bytes != sizeof(HeaderMessage)) {
      printf("Header length invalid! Closing connection with socket %d\n", new_fd);

      mark_player_inactive(player_index);
      close(new_fd);

      return NULL;
    }

    // make sure the header is recieved in full in case of connection
    // interruptions
    if (bytes == sizeof(HeaderMessage)) {
      HeaderType header_type = ntohs(header.type);
      uint16_t length = ntohs(header.length);

      // char message[500];

      // sprintf(message, "Type of message is %d\n", header_type);
      // if (send(new_fd, message, strnlen(message, 500), 0) == -1) perror("send");

      // sprintf(message, "Length of message is %d\n", length);
      // if (send(new_fd, message, strnlen(message, 500), 0) == -1) perror("send");

      if (header_type == MOVE_ACTION) {
        // ---- move action
        if (length > sizeof(MoveAction)) {
          printf("Invalid move action! Closing connection with socket %d\n", new_fd);

          mark_player_inactive(player_index);
          close(new_fd);

          return NULL;
        } else {
          MoveAction action;
          ssize_t move_bytes = recv(new_fd, &action, sizeof(MoveAction), MSG_WAITALL);

          if (move_bytes == -1) {
            perror("move action recv");
            continue;
          }

          int16_t x = (int16_t)ntohs((uint16_t)action.x);
          int16_t y = (int16_t)ntohs((uint16_t)action.y);

          // sprintf(message, "I am moving you an additional x=%hd and y=%hd\n", x, y);
          // if (send(new_fd, message, strnlen(message, 500), 0) == -1) perror("send");

          move_player_position(player_index, x, y);

          // stream the new player positiion to everyone
          // stream_player_position(player_index);

          HeaderMessage move_action_stream_header;
          move_action_stream_header.length = htons(sizeof(PlayerPositionUpdatePacket));
          move_action_stream_header.type = htons(PLAYER_MOVE_UPDATE);  // position stream type

          pthread_mutex_lock(&players_mutex);

          PlayerPositionUpdatePacket packet;
          packet.player_index = (int16_t)htons((uint16_t)player_index);
          packet.x = (int16_t)htons((uint16_t)players[player_index].x);
          packet.y = (int16_t)htons((uint16_t)players[player_index].y);

          for (int i = 0; i < MAX_PLAYERS; i++) {
            // stream to all active players except current player
            if (players[i].active && i != player_index) {
              printf("Sending to %d\n", player_index);
              if (send(players[i].sock_fd, &move_action_stream_header, sizeof(HeaderMessage), 0) ==
                  -1)
                perror("send");

              if (send(players[i].sock_fd, &packet, sizeof(PlayerPositionUpdatePacket), 0) == -1)
                perror("send");
            }
          }

          pthread_mutex_unlock(&players_mutex);
        }
      } else if (header_type == CHAT) {
        // ----- chat action
        char data_buffer[500];
        // next course of action
        // do another recv call here to actually parse the data
        if (length > 500) {
          printf("Payload too large! Disconnecting malicious client.\n");
          // handle error/close connection

          mark_player_inactive(player_index);
          close(new_fd);

          return NULL;
        } else {
          recv(new_fd, data_buffer, length, MSG_WAITALL);
        }
      } else {
        if (send(new_fd, "Not sure what you're tryna tell me bud.\n", 40, 0) == -1) perror("send");
      }
    }
  }

  return NULL;
}
