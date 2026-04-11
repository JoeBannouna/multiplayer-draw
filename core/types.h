#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum : uint16_t { MOVE_ACTION, CHAT, PLAYER_MOVE_UPDATE, PLAYER_JOIN_EVENT, PLAYER_LEAVE_EVENT } HeaderType;

typedef struct __attribute__((packed)) {
  uint16_t type;
  uint16_t length;
} HeaderMessage;

typedef struct __attribute__((packed)) {
  int16_t x;
  int16_t y;
} MoveAction;

typedef struct __attribute__((packed)) {
  int16_t x;
  int16_t y;
  int16_t player_index;
} PlayerPositionUpdatePacket;

#define MAX_HEADER_LENGTH sizeof(PlayerPositionUpdatePacket)

#endif
