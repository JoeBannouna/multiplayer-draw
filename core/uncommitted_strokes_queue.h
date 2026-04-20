#ifndef UNCOMMITTED_STROKES_QUEUE_H
#define UNCOMMITTED_STROKES_QUEUE_H

#include <pthread.h>

// #include "object_pool.h"
#include "stroke_manager.h"
#include "types.h"

#include "dynamic_arr.h"

typedef struct {
  // an array of pointers to uncommittedStroke objects in fragmented memory
  UncommittedStroke** array;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  uint16_t capacity;  // 2 bytes
  uint16_t len;       // 2 bytes
  uint16_t begin;     // 2 bytes
  EntryList active_strokes_list;
} UncommittedStrokesQueue;

void USQ_initalize(UncommittedStrokesQueue* q, uint16_t capacity);
void USQ_grow(UncommittedStrokesQueue* q);
// remove the active index marker and pass every stroke as the active stroke in SM (stroke manager)
void USQ_drain(UncommittedStrokesQueue* q, StrokeManager* stroke_manager);
// marks it as the active index for that player
UncommittedStroke* USQ_enqueue(UncommittedStrokesQueue* q, Stroke stroke, int16_t unique_player_index);
void USQ_add_point(UncommittedStroke* uncommitted_stroke_ptr, StrokePoint point);
void USQ_destroy(UncommittedStrokesQueue* q);
// go to the previous uncommitted stroke pointed to by the uncommitted stroke struct
void USQ_undo_stroke(UncommittedStrokesQueue* q, StrokeManager* manager, uint16_t unique_player_index);
// go to the next uncommitted stroke pointed to by the uncommitted stroke struct
void USQ_redo_stroke(UncommittedStrokesQueue* q, StrokeManager* manager, uint16_t unique_player_index);

#endif
