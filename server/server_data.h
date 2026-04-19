#ifndef SERVER_DATA_H
#define SERVER_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int16_t players_len;
  int16_t players_capacity;
  int8_t* name_lens;
  char** names;
} ServerData;

void SD_dump_memory(ServerData* server_data, FILE* fptr);
void SD_reconstruct_memory(ServerData* server_data, FILE* fptr);
bool SD_double_capacity(ServerData* server_data);
// returns the index of the newly added player
int16_t SD_add_player(ServerData* server_data, const char* name, int8_t name_len);
int16_t SD_get_player_unique_index(ServerData* server_data, const char* name, int8_t name_len);

#endif
