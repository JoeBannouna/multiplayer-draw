#include "uncommitted_strokes_queue.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "stroke_manager.h"
#include "types.h"

void USQ_initalize(UncommittedStrokesQueue* q, uint16_t capacity) {
  pthread_mutex_init(&q->mutex, NULL);
  pthread_cond_init(&q->cond, NULL);

  q->begin = 0;
  q->len = 0;

  q->array = malloc(capacity * sizeof(UncommittedStroke*));
  if (q->array == NULL) {
    printf("UncommittedStrokesQueue malloc failed!\n");
    exit(1);
  } else printf("Allocated q of total array size %ld bytes\n", capacity * sizeof(MoveAction));

  q->capacity = capacity;
}

void USQ_grow(UncommittedStrokesQueue* q) {
  uint16_t new_cap = q->capacity * 2;
  UncommittedStroke** new_buf = malloc(new_cap * sizeof(UncommittedStroke*));

  // Copy in logical order, unwrapping the ring
  for (int i = 0; i < q->len; i++) new_buf[i] = q->array[(q->begin + i) % q->capacity];

  free(q->array);
  q->array = new_buf;
  q->begin = 0;
  // q->end = q->len  (already the right position)
  q->capacity = new_cap;
}

void USQ_drain(UncommittedStrokesQueue* q, StrokeManager* stroke_manager) {
  // while (first element in queue has finished=true) {
  //    dequeue()
  // }

  while (q->len > 0 && q->array[q->begin]->finished == true) {
    printf("Freeing a stroke\n");
    //  dequeue process

    UncommittedStroke* uncommitted_stroke_ptr = q->array[q->begin];

    q->begin = (uint16_t)(q->begin + 1) % q->capacity;
    q->len--;

    SM_add_uncommitted_stroke(stroke_manager, uncommitted_stroke_ptr);
    // printf("%p\n", stroke_manager);

    free(uncommitted_stroke_ptr->points);
    free(uncommitted_stroke_ptr);
  }
}

UncommittedStroke* USQ_enqueue(UncommittedStrokesQueue* q, Stroke stroke) {
  // returns the index of the stroke??
  if (q->len == q->capacity) { USQ_grow(q); }

  // Find the next available slot using wrap-around
  int target_index = (q->begin + q->len) % q->capacity;

  // bounds checking
  if (target_index >= q->capacity || target_index < 0) {
    printf(
        "You tried to enqueue at an index=%d (outside the memory allocated). "
        "\nExiting.\n ",
        target_index
    );
    exit(1);
  }

  UncommittedStroke* new_uncommitted_stroke_ptr = malloc(sizeof(UncommittedStroke));
  new_uncommitted_stroke_ptr->stroke = stroke;
  uint32_t points_capacity = 32;
  new_uncommitted_stroke_ptr->points = malloc(points_capacity * sizeof(StrokePoint));
  new_uncommitted_stroke_ptr->points_capacity = points_capacity;
  new_uncommitted_stroke_ptr->finished = false;

  // TODO: need to do malloc NULL check here

  q->array[target_index] = new_uncommitted_stroke_ptr;
  q->len++;

  return new_uncommitted_stroke_ptr;
}

void USQ_add_point(UncommittedStroke* uncommitted_stroke_ptr, StrokePoint point) {
  // UncommittedStroke uncommitted_stroke = *uncommitted_stroke_ptr;
  if (uncommitted_stroke_ptr->stroke.points_len == uncommitted_stroke_ptr->points_capacity) {
    StrokePoint* res = realloc(
        uncommitted_stroke_ptr->points,
        uncommitted_stroke_ptr->points_capacity * 2 * sizeof(StrokePoint)
    );
    if (res != NULL) {
      uncommitted_stroke_ptr->points = res;
      uncommitted_stroke_ptr->points_capacity *= 2;
    } else {
      printf("Realloc returned null???\n");
      exit(1);
    }
  }

  uncommitted_stroke_ptr->points[uncommitted_stroke_ptr->stroke.points_len] = point;
  uncommitted_stroke_ptr->stroke.points_len = uncommitted_stroke_ptr->stroke.points_len + 1;
}

void USQ_destroy(UncommittedStrokesQueue* q) {
  pthread_mutex_destroy(&q->mutex);
  pthread_cond_destroy(&q->cond);
  // TODO: free(q->array); This needs to also free all the uncomitted stroke pointers
  // that may exist in the queue! along with their points, etc..
}

// TODO: remove this function as it probably doesn't need to exist
void USQ_loop_strokes(UncommittedStrokesQueue* q) {
  for (size_t i = 0; i < q->len; i++) {
    printf("Yah %p\n", q->array[(q->begin + i) % q->capacity]);
  }
}

void USQ_undo_stroke(UncommittedStrokesQueue* q, uint16_t player_index) {
  // Iterate backwards from the most recently added stroke
  printf("%u\n", player_index);
    printf("%p\n", q);
  // for (size_t j = 0; j < q->len; j++) {
  //   size_t i = q->len - 1 - j;
  //   printf("%lu\n", i);

  //   // UncommittedStroke* curr = q->array[(q->begin + i) % q->capacity];
  //   // printf("%p\n", curr);

  //   // if (curr->stroke.player_index != player_index) continue;

  //   // if (!curr->stroke.undo) {
  //   //   // Case 2: found the most recent undoable stroke for this player
  //   //   Stroke* stroke_ptr = &curr->stroke;
  //   //   stroke_ptr->undo = true;
  //   //   return;
  //   // }
  // }

  // Case 1 & 3: no undoable uncommitted stroke found — fall back to committed
  // USQ_undo_committed_stroke(q, player_index); // or however you call it
  // printf("Undid something with player index = %d\n", player_index);
  return;
}
