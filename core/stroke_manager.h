#ifndef STROKE_MANAGER_H
#define STROKE_MANAGER_H

#include <pthread.h>

#include "types.h"
#include "dynamic_arr.h"

// manages strokes for a single layer in an art work
// strokes from multiple players are stored
// an artwork has a stroke manager for every layer it contains
typedef struct {
  Stroke* strokes;
  StrokePoint* points;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  uint32_t strokes_capacity;  // 4 bytes
  uint32_t points_capacity;   // 4 bytes
  uint32_t strokes_len;       // 4 bytes
  uint32_t points_len;        // 4 bytes
  // uint16_t* last_stroke_index_for_players;
  SList active_strokes_list;
} StrokeManager;

void SM_initalize(StrokeManager* manager, uint32_t initial_points_capacity);
void SM_destroy(StrokeManager* manager);
bool SM_is_empty(StrokeManager* manager);
// returns the index of the newly added stroke
int32_t SM_add_uncommitted_stroke(StrokeManager* manager, UncommittedStroke* uncommitted_stroke_ptr);
void SM_undo_stroke(StrokeManager* manager, uint16_t player_index);
UncommittedStroke* SM_redo_stroke(StrokeManager* manager, uint16_t player_index);
void SM_point_active_stroke_to(StrokeManager* manager, UncommittedStroke* new_uncommitted_stroke_ptr, int16_t unique_player_index);


// add_player_to_last_stroke_buffer();
// remove_player_from_shared_stroke_buffer();

#endif
