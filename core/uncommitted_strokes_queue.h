#ifndef UNCOMMITTED_STROKES_QUEUE_H
#define UNCOMMITTED_STROKES_QUEUE_H

#include <pthread.h>

#include "stroke_manager.h"
#include "types.h"

typedef struct {
  // an array of pointers to uncommittedStroke objects in fragmented memory
  UncommittedStroke** array;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  uint16_t capacity;  // 2 bytes
  uint16_t len;       // 2 bytes
  uint16_t begin;     // 2 bytes
} UncommittedStrokesQueue;

// typedef struct {
//   // queues[player_index] := the queue of strokes for that player
//   UncommittedStrokesQueue* queues;
//   // at[player_index] := the nth element the queue (current active stroke)
//   int32_t* active_stroke_index_for_players;
// } PlayerUncommittedStrokesQueue;

void USQ_initalize(UncommittedStrokesQueue* q, uint16_t capacity);
void USQ_grow(UncommittedStrokesQueue* q);
void USQ_drain(UncommittedStrokesQueue* q, StrokeManager* stroke_manager);
UncommittedStroke* USQ_enqueue(UncommittedStrokesQueue* q, Stroke stroke);
void USQ_add_point(UncommittedStroke* uncommitted_stroke_ptr, StrokePoint point);
void USQ_destroy(UncommittedStrokesQueue* q);
void USQ_loop_strokes(UncommittedStrokesQueue* q);
void USQ_undo_stroke(UncommittedStrokesQueue* q, StrokeManager* manager, uint16_t player_index);

#endif
