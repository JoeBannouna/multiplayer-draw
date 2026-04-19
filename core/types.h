#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum : uint16_t {
  MOVE_ACTION,
  CHAT,
  PLAYER_MOVE_UPDATE,
  PLAYER_JOIN_EVENT,
  PLAYER_LEAVE_EVENT,
  PLAYER_STROKE_START,
  PLAYER_STROKE_END,
  PLAYER_POINT_ADD,
  PLAYER_INDEX_ASSIGNMENT,
} HeaderType;

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

typedef struct __attribute__((packed)) {
  int16_t x;
  int16_t y;
  uint16_t radius;
  uint8_t opacity;
} StrokePoint;

typedef struct __attribute__((packed)) {
  uint32_t points_len;
  uint16_t shape;  // allows for different brush types in the future ig
  uint16_t player_index;
  uint8_t color_r;
  uint8_t color_b;
  uint8_t color_g;
  bool undo;
} Stroke;

typedef struct {
  Stroke stroke;  // contains the points_len number here
  StrokePoint* points;
  uint32_t points_capacity;
  bool finished;
} UncommittedStroke;

#define MAX_HEADER_LENGTH sizeof(PlayerPositionUpdatePacket)
#define MAX_CHAT_SIZE 500

#endif
