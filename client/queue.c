#include "queue.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void QU_initalize(Queue* queue, uint16_t capacity) {
  // queue->mutex = PTHREAD_MUTEX_INITIALIZER;
  // queue->cond = PTHREAD_COND_INITIALIZER;

  pthread_mutex_init(&queue->mutex, NULL);
  pthread_cond_init(&queue->cond, NULL);

  queue->begin = 0;
  queue->end = 0;
  queue->len = 0;

  queue->array = malloc(capacity * sizeof(MoveAction));
  if (queue->array == NULL) {
    printf("Queue malloc failed!\n");
    exit(1);
  } else printf("Allocated queue of total array size %ld bytes\n", capacity * sizeof(MoveAction));

  queue->capacity = capacity;
}

void QU_destroy(Queue* queue) {
  pthread_mutex_destroy(&queue->mutex);
  pthread_cond_destroy(&queue->cond);
  free(queue->array);
}

// enqueue at end of the queue
void QU_enqueue(Queue* queue, const MoveAction item) {
  if (queue->len == queue->capacity) {
    // Queue is full! (Handle error or resize)
    printf("Tried enqueing while capacity reached, you did something wrong");
    exit(1);
    return;
  }

  // Find the next available slot using wrap-around
  int target_index = (queue->begin + queue->len) % queue->capacity;

  // bounds checking
  if (target_index >= queue->capacity || target_index < 0) {
    printf(
        "You tried to enqueue at an index=%d (outside the memory allocated). "
        "\nExiting.\n ",
        target_index
    );
    exit(1);
  }

  queue->array[target_index] = item;
  queue->len++;
}

bool QU_is_empty(Queue* queue) { return queue->len == 0; }

bool QU_is_full(Queue* queue) { return queue->len == queue->capacity; }

// dequeue from the beginning of the queue
MoveAction QU_dequeue(Queue* queue) {
  if (queue->len == 0) {
    printf("Tried dequeing while queue is empty, you did something wrong");
    exit(1);
  }

  MoveAction target = queue->array[queue->begin];

  queue->begin = (uint16_t)(queue->begin + 1) % queue->capacity;
  queue->len--;

  return target;
}

// peak the beginning of the queue (probably not needed)
MoveAction QU_peek(Queue* queue) { return queue->array[queue->begin]; }
