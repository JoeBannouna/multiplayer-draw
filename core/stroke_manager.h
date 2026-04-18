#ifndef STROKE_MANAGER_H
#define STROKE_MANAGER_H

#include <pthread.h>
#include <string.h>

#include "types.h"

// manages strokes for a single layer in an art work
// strokes from multiple players are stored
// an artwork has a stroke manager for every layer it contains
typedef struct {
  Stroke* strokes;
  StrokePoint* points;
  uint32_t* point_to_stroke_index;
  uint32_t* point_to_player_index;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  uint32_t strokes_capacity;  // 4 bytes
  uint32_t points_capacity;   // 4 bytes
  uint32_t strokes_len;       // 4 bytes
  uint32_t points_len;        // 4 bytes
} StrokeManager;

void SM_initalize(StrokeManager* manager, uint32_t initial_points_capacity);
void SM_destroy(StrokeManager* manager);
// void SM_add_stroke(StrokeManager* manager, const Stroke item);
// void SM_add_point(StrokeManager* manager, uint32_t stroke_index, StrokePoint point);
bool SM_is_empty(StrokeManager* manager);
void SM_undo_stroke(StrokeManager* manager, int16_t player_index);
void SM_redo_stroke(StrokeManager* manager, int16_t player_index);
void SM_add_uncommitted_stroke(StrokeManager* manager, UncommittedStroke* uncommitted_stroke_ptr);

// add_player_to_shared_stroke_buffer();
// remove_player_from_shared_stroke_buffer();

#endif
