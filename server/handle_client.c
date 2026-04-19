#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// all header files
#include "../core/network_utils.h"
#include "../core/types.h"
#include "player.h"
#include "server_data.h"

// global variable definitions
#include "state.c"

// Handles client actions
// returns 'true' on successful handlings
// returns 'false' to indicate an error occured and thus the thread needs to be terminated
bool handle_client_packet(int playerfd, int16_t player_index, HeaderMessage* header) {
  HeaderType recieved_header_type = ntohs(header->type);
  uint16_t recieved_header_length = ntohs(header->length);

  HeaderMessage send_header;

  switch (recieved_header_type) {
    case MOVE_ACTION:
      if (recieved_header_length != sizeof(MoveAction)) {
        printf("Invalid header length! Closing connection with socket %d\n", playerfd);
        return false;
      }
      MoveAction action;
      ssize_t bytes = recv_exact(playerfd, &action, sizeof(MoveAction));

      // connection closed
      if (bytes == 1) {
        printf("Connection closed in the move action?\n");
        return false;
      }

      if (bytes == -1) {
        perror("move action recv");
        // continue;
        // i think return true here to let the loop continue & ignore this action that had an
        // error??
        return true;
      }

      move_player_position(
          player_index, (int16_t)ntohs((uint16_t)action.x), (int16_t)ntohs((uint16_t)action.y)
      );

      // stream the new player positiion to everyone
      send_header.length = htons(sizeof(PlayerPositionUpdatePacket));
      send_header.type = htons(PLAYER_MOVE_UPDATE);  // position stream type

      pthread_mutex_lock(&players_mutex);
      PlayerPositionUpdatePacket packet;
      packet.player_index = (int16_t)htons((uint16_t)player_index);
      packet.x = (int16_t)htons((uint16_t)players[player_index].x);
      packet.y = (int16_t)htons((uint16_t)players[player_index].y);
      pthread_mutex_unlock(&players_mutex);

      stream_player_event(player_index, &send_header, &packet, sizeof(PlayerPositionUpdatePacket));

      return true;
    case CHAT:
      if (recieved_header_length > MAX_CHAT_SIZE) {
        printf("Message too large! Disconnecting malicious client.\n");
        return false;
      }

      char data_buffer[500];
      bytes = recv_exact(playerfd, data_buffer, recieved_header_length);

      // connection closed
      if (bytes == 1) {
        printf("Connection closed in the chat action?\n");
        return false;
      }

      if (bytes == -1) {
        perror("chat action recv");
        return true;
      }

      return true;
    default:
      if (send(playerfd, "Not sure what you're tryna tell me bud.\n", 40, 0) == -1) perror("send");
      return false;
  }

  return true;
}

void* handle_client(void* new_fd_p) {
  int playerfd = *((int*)new_fd_p);
  free((int*)new_fd_p);

  printf("NEW CLIENT CONNECTION RECIEVED\n");

  // register player
  int16_t player_index = register_player(playerfd);

  if (player_index == -1) {
    printf("All player spots have been filled. No space for this connection!\n");
    printf("Closing connection.\n");

    close(playerfd);
    return NULL;
  }

  print_players();

  // /// --- SEND THE NEW CLIENT HIS INDEX BACK TO HIM ---
  // HeaderMessage header_message = {
  //     .length = htons(sizeof(int16_t)), .type = htons(PLAYER_INDEX_ASSIGNMENT)
  // };

  // if (send(players[player_index].sock_fd, &header_message, sizeof(HeaderMessage), 0) == -1)
  //   perror("send");
  // if (send(players[player_index].sock_fd, &player_index, sizeof(int16_t), 0) == -1)
  // perror("send");

  /// --- RECEIVE THE PLAYER NAME ---
  HeaderMessage received_header;
  ssize_t bytes = 0;
  bytes = recv_exact(playerfd, &received_header, sizeof(HeaderMessage));

  // format to host format
  received_header =
      (HeaderMessage){.length = ntohs(received_header.length), .type = ntohs(received_header.type)};

  // TODO: Add something like max_username_length to config instead of hardcoded 50
  if (received_header.type != PLAYER_USERNAME_ASSIGNMENT || received_header.length > 50) {
    printf("Thread is terminating.\n");
    mark_player_inactive(player_index);
    close(playerfd);

    return NULL;
  }

  // save or find the user
  // TODO: hardcoded username buffer length
  char username_buffer[50];
  bytes = recv_exact(playerfd, username_buffer, received_header.length * sizeof(char));

  int16_t player_unique_index =
      SD_get_player_unique_index(&server_data, username_buffer, received_header.length);
  int16_t player_unique_index_network_format = htons(player_unique_index);

  /// --- SEND THE NEW CLIENT HIS INDEX BACK TO HIM ---
  HeaderMessage header_message = {
      .length = htons(sizeof(int16_t)), .type = htons(PLAYER_INDEX_ASSIGNMENT)
  };

  if (send(players[player_index].sock_fd, &header_message, sizeof(HeaderMessage), 0) == -1)
    perror("send");

  // if (send(players[player_index].sock_fd, &player_index, sizeof(int16_t), 0) == -1)
  // perror("send");
  if (send(
          players[player_index].sock_fd, &player_unique_index_network_format, sizeof(int16_t), 0
      ) == -1)
    perror("send");

  /// --- STREAMING PLAYER JOIN EVENT TO ALL OTHER PLAYERS AND
  /// STREAMING JOIN EVENTS TO THE CURRENT PLAYER FOR
  /// EVERY EXISTING PLAYER ---

  // stream the new player join to other players
  header_message = (HeaderMessage){
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

  /// ---------

  /// --- ACCEPTING ACTIONS FROM PLAYERS ----
  // HeaderMessage received_header;
  // ssize_t bytes = 0;
  while (true) {
    bytes = recv_exact(playerfd, &received_header, sizeof(HeaderMessage));

    if (bytes == -1) {
      perror("client recv");
      continue;
    }

    /// --- CONNECTION CLOSED...
    /// STREAM "PLAYER LEFT" EVENT TO ALL EXISTING PLAYERS ---
    if (bytes == 1) {
      printf("Goodbye, %d\n", playerfd);
      break;
    }

    if (!handle_client_packet(playerfd, player_index, &received_header)) break;
  }

  // stream leaving to other players
  printf("Streaming to other players that %hd left\n", player_index);
  uint16_t payload = (uint16_t)htons((uint16_t)player_index);
  HeaderMessage leave_header = {
      .type = htons(PLAYER_LEAVE_EVENT), .length = htons(sizeof(payload))
  };
  stream_player_event(player_index, &leave_header, &payload, sizeof(payload));

  printf("Thread is terminating.\n");
  mark_player_inactive(player_index);
  close(playerfd);

  return NULL;
}
