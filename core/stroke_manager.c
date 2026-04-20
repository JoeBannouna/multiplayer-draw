
#include "stroke_manager.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

void SM_initalize(StrokeManager* manager, uint32_t initial_points_capacity) {
  pthread_mutex_init(&manager->mutex, NULL);
  pthread_cond_init(&manager->cond, NULL);

  manager->strokes_len = 0;
  manager->points_len = 0;
  manager->strokes_capacity = 128;
  manager->points_capacity = initial_points_capacity;

  manager->strokes = malloc(manager->strokes_capacity * sizeof(Stroke));
  if (manager->strokes == NULL) {
    printf("Strokes manager malloc failed!\n");
    exit(1);
  } else
    printf(
        "Allocated strokes of total array size %ld bytes\n",
        manager->strokes_capacity * sizeof(Stroke)
    );

  // manager->point_to_stroke_index = malloc(manager->points_capacity * sizeof(uint32_t));
  manager->points = malloc(manager->points_capacity * sizeof(StrokePoint));
  // if (manager->points == NULL || manager->point_to_stroke_index == NULL) {
  if (manager->points == NULL) {
    printf("Strokes manager malloc failed!\n");
    exit(1);
  } else
    printf(
        "Allocated stroke points of total array size %ld bytes\n",
        manager->points_capacity * sizeof(StrokePoint)
    );
}

void double_strokes_capacity(StrokeManager* manager) {
  Stroke* tmp_allocated_strokes =
      realloc(manager->strokes, manager->strokes_capacity * 2 * sizeof(Stroke));
  if (tmp_allocated_strokes != NULL) {
    manager->strokes_capacity = manager->strokes_capacity * 2;
    manager->strokes = tmp_allocated_strokes;
  } else {
    printf("Strokes manager realloc failed!\n");
    exit(1);
  }
}

void double_points_capacity(StrokeManager* manager) {
  // Resize to hold 10 integers
  StrokePoint* tmp_points =
      realloc(manager->points, manager->points_capacity * 2 * sizeof(StrokePoint));
  // uint32_t* tmp_point_stroke_id =
  //     realloc(manager->point_to_stroke_index, manager->points_capacity * 2 * sizeof(uint32_t));
  // if (tmp_points != NULL && tmp_point_stroke_id != NULL) {
  if (tmp_points != NULL) {
    manager->points_capacity = manager->points_capacity * 2;
    manager->points = tmp_points;
    // manager->point_to_stroke_index = tmp_point_stroke_id;
  } else {
    printf("Strokes manager points realloc failed!\n");
    exit(1);
  }
}

void SM_destroy(StrokeManager* manager) {
  pthread_mutex_destroy(&manager->mutex);
  pthread_cond_destroy(&manager->cond);
  free(manager->strokes);
  free(manager->points);
  // free(manager->point_to_stroke_index);
}

// void SM_add_stroke(StrokeManager* manager, const Stroke item) {
//   if (manager->strokes_len == manager->strokes_capacity) { double_strokes_capacity(manager); }

//   manager->strokes[manager->strokes_len] = item;
//   manager->strokes_len++;
// }

// void SM_add_point(StrokeManager* manager, uint32_t stroke_index, const StrokePoint point) {
//   if (manager->points_len == manager->points_capacity) {
//     printf("Doubling point capacity\n");
//     double_points_capacity(manager);
//   }

//   manager->points[manager->points_len] = point;
//   manager->point_to_stroke_index[manager->points_len] = stroke_index;
//   manager->strokes[stroke_index].points_len++;
//   manager->points_len++;
// }

bool SM_is_empty(StrokeManager* manager) { return manager->strokes_len == 0; }

void SM_undo_stroke(StrokeManager* manager, uint16_t player_index) {
  // TODO: store a hashmap that goes like: player_index -> array_of_stroke_indexes
  // find the last stroke from the player
  for (size_t j = 0; j < manager->strokes_len; j++) {
    size_t i = manager->strokes_len - 1 - j;  // the actual index

    if (manager->strokes[i].player_index == player_index) {
      // last_stroke_from_player_index = (int32_t)i;

      if (manager->strokes[i].undo == false) {
        manager->strokes[i].undo = true;
        break;
      }
    }
  }
}

void SM_add_uncommitted_stroke(StrokeManager* manager, UncommittedStroke* uncommitted_stroke_ptr) {
  assert(manager != NULL);
  assert(uncommitted_stroke_ptr != NULL);
  assert(uncommitted_stroke_ptr->points != NULL);
  assert(uncommitted_stroke_ptr->stroke.points_len > 0);

  if (manager->strokes_len == manager->strokes_capacity) { double_strokes_capacity(manager); }
  // if (manager->points_len == manager->points_capacity) { double_points_capacity(manager); }
  while (manager->points_len + uncommitted_stroke_ptr->stroke.points_len >
         manager->points_capacity) {
    double_points_capacity(manager);
  }

  manager->strokes[manager->strokes_len] = uncommitted_stroke_ptr->stroke;
  memcpy(
      manager->points + manager->points_len, uncommitted_stroke_ptr->points,
      uncommitted_stroke_ptr->stroke.points_len * sizeof(StrokePoint)
  );

  manager->strokes_len++;
  manager->points_len += uncommitted_stroke_ptr->stroke.points_len;
}
