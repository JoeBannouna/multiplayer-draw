#include "server_data.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "state.c"

void SD_dump_memory(ServerData* server_data, FILE* fptr) {
  fprintf(fptr, "%d\n", server_data->players_len);
  for (int16_t i = 0; i < server_data->players_len; i++) {
    fprintf(fptr, "%.*s\n", server_data->name_lens[i], server_data->names[i]);
  }
}

void SD_reconstruct_memory(ServerData* server_data, FILE* fptr) {
  // Store the content of the file
  char tmp[100];
  fgets(tmp, 100, fptr);
  char* endptr;
  int16_t players_len = strtol(tmp, &endptr, 10);
  // TODO: handle errors
  if (*endptr != '\0') { /* Handle error: not all characters were numeric */
  }

  if (players_len == 0) server_data->players_capacity = 2;
  else server_data->players_capacity = players_len;

  // start with length zero and the while loop will take care of adding player
  server_data->players_len = 0;

  server_data->name_lens = malloc(server_data->players_capacity * 2 * sizeof(int8_t));
  server_data->names = malloc(server_data->players_capacity * 2 * sizeof(char*));

  if (server_data->names == NULL || server_data->name_lens == NULL) {
    printf("Server data malloc failed!\n");
    exit(1);
  }

  // Read the content and print it
  while (fgets(tmp, 100, fptr)) {
    // subtracted one for the \n newline character after every name
    int8_t len = strnlen(tmp, INT8_MAX) - 1;
    printf("%.*s of lenth %d\n", len, tmp, len);
    SD_add_player(server_data, tmp, len);
  }

  // fprintf(fptr, "%d\n", server_data->players_len);
  for (int16_t i = 0; i < server_data->players_len; i++) {
    printf("i %d\n", i);
    printf("%.*s\n", server_data->name_lens[i], server_data->names[i]);
  }
}

bool SD_double_capacity(ServerData* server_data) {
  int8_t* tmp_lens =
      realloc(server_data->name_lens, server_data->players_capacity * 2 * sizeof(int8_t));
  char** tmp_names = realloc(server_data->names, server_data->players_capacity * 2 * sizeof(char*));
  if (tmp_lens != NULL && tmp_names != NULL) {
    server_data->players_capacity *= 2;
    server_data->names = tmp_names;
    server_data->name_lens = tmp_lens;
  } else {
    // idk if this is enougn or not
    // TODO: figure out if this needs to be re-engineered
    printf("Server data realloc failed!\n");
    exit(1);
  }

  return true;
}

int16_t SD_add_player(ServerData* server_data, const char* name, int8_t name_len) {
  if (server_data->players_len == server_data->players_capacity) {
    SD_double_capacity(server_data);
  }

  server_data->names[server_data->players_len] = malloc(name_len * sizeof(char));
  strncpy(server_data->names[server_data->players_len], name, name_len);
  server_data->name_lens[server_data->players_len] = name_len;

  server_data->players_len++;

  return server_data->players_len - 1;
}

int16_t SD_get_player_unique_index(ServerData* server_data, const char* name, int8_t name_len) {
  ssize_t player_unique_index = -1;
  for (int16_t i = 0; i < server_data->players_len; i++) {
    char* curr_name = server_data->names[i];
    // check if they are equal
    if (server_data->name_lens[i] == name_len) {
      if (strncmp(curr_name, name, name_len) == 0) { player_unique_index = i; }
    }
  }

  if (player_unique_index == -1) {
    player_unique_index = SD_add_player(server_data, name, name_len);

    FILE* fptr = fopen(global_argv[1], "w");
    SD_dump_memory(server_data, fptr);
    fclose(fptr);
  }

  return player_unique_index;
}
