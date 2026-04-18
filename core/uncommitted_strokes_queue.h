#ifndef UNCOMMITTED_STROKES_QUEUE_H
#define UNCOMMITTED_STROKES_QUEUE_H

#include <pthread.h>

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

#endif
