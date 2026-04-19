#ifndef QUEUE_H
#define QUEUE_H

#include "../core/types.h"
#include <pthread.h>

typedef struct {
  MoveAction* array;  // 8 bytes
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  uint16_t capacity;  // 2 bytes
  uint16_t len;       // 2 bytes
  uint16_t begin;     // 2 bytes
  uint16_t end;       // 2 bytes
} Queue;

void QU_initalize(Queue* queue, uint16_t capacity);
void QU_destroy(Queue* queue);
void QU_enqueue(Queue* queue, const MoveAction item);
bool QU_is_empty(Queue* queue);
bool QU_is_full(Queue* queue);
MoveAction QU_dequeue(Queue* queue);
MoveAction QU_peek(Queue* queue);

#endif
