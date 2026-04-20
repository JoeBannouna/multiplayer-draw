#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "uncommitted_strokes_queue.h"

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
  PLAYER_USERNAME_ASSIGNMENT,
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

// typedef struct __attribute__((packed)) {
//   int16_t x;
//   int16_t y;
//   uint16_t radius;
//   uint8_t opacity;
// } StrokePoint;

// typedef struct __attribute__((packed)) {
//   uint16_t points_len;
//   uint16_t previous_stroke_index; // for undos and redos
//   uint16_t next_stroke_index; // for undos and redos
//   uint16_t shape;  // allows for different brush types in the future ig
//   uint8_t color_r;
//   uint8_t color_b;
//   uint8_t color_g;
//   bool undo;
// } Stroke;

typedef struct {
  int16_t x;
  int16_t y;
  uint16_t radius;
  uint8_t opacity;
  uint8_t _pad;  // explicit, documents intent
} StrokePoint;

typedef struct {
  uint16_t points_len;
  uint16_t previous_stroke_index;
  uint16_t next_stroke_index;
  uint16_t shape;
  uint8_t color_r;
  uint8_t color_g;
  uint8_t color_b;
  bool undo;
} Stroke;

// Wire format — packed, explicit, only used at the serialization boundary
typedef struct __attribute__((packed)) {
  int16_t x;
  int16_t y;
  uint16_t radius;
  uint8_t opacity;
} WireStrokePoint;

typedef struct __attribute__((packed)) {
  uint16_t points_len;
  uint16_t previous_stroke_index;
  uint16_t next_stroke_index;
  uint16_t shape;
  uint8_t color_r;
  uint8_t color_g;
  uint8_t color_b;
  uint8_t undo;  // bool → uint8_t for portability
} WireStroke;



typedef struct UncommittedStroke {
  StrokePoint* points;
  struct UncommittedStroke* next;
  struct UncommittedStroke* prev;
  Stroke stroke;  // contains the points_len number here
  uint32_t points_capacity;
  uint32_t unique_player_index;
  bool finished;
} UncommittedStroke;

#define MAX_HEADER_LENGTH sizeof(PlayerPositionUpdatePacket)
#define MAX_CHAT_SIZE 500

#endif
