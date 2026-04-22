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

  SList_init(&manager->active_strokes_list);

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

bool SM_is_empty(StrokeManager* manager) { return manager->strokes_len == 0; }

void SM_undo_stroke(StrokeManager* manager, uint16_t unique_player_index) {
  // // TODO: store a hashmap that goes like: player_index -> array_of_stroke_indexes
  // // find the last stroke from the player
  // for (size_t j = 0; j < manager->strokes_len; j++) {
  //   size_t i = manager->strokes_len - 1 - j;  // the actual index

  //   if (manager->strokes[i].player_index == player_index) {
  //     // last_stroke_from_player_index = (int32_t)i;

  //     if (manager->strokes[i].undo == false) {
  //       manager->strokes[i].undo = true;
  //       break;
  //     }
  //   }
  // }

  SEntry* entry = SList_find_entry(&manager->active_strokes_list, unique_player_index);
  if (entry != NULL) {
    printf(
        "There is an active entry in SM, going back if it's not the last one, and deleting it if "
        "it is\n"
    );

    if (entry->data.stroke_index != UINT32_MAX) {
      printf("Currently, stroke of index %d will be undo=true\n", entry->data.stroke_index);
      manager->strokes[entry->data.stroke_index].undo = true;
    }

    if (manager->strokes[entry->data.stroke_index].previous_stroke_index == UINT16_MAX) {
      // SList_remove_entry(&manager->active_strokes_list, unique_player_index);
      // instead of removing the entry, just update it to point at the

      // SEntry updated_entry = (SEntry){
      //     .unique_player_index = unique_player_index,
      //     .data = (StrokeContainer){
      //         .stroke_index = INT32_MAX,
      //         .uncommitted_stroke_link = entry->data.uncommitted_stroke_link
      //     }
      // };

      // SList_update_entry(&manager->active_strokes_list, &updated_entry);

      // changing strats again, just leaving it as is
    } else {
      SEntry updated_entry = (SEntry){
          .unique_player_index = unique_player_index,
          .data = (StrokeContainer){
              .stroke_index = manager->strokes[entry->data.stroke_index].previous_stroke_index,
              .uncommitted_stroke_link = entry->data.uncommitted_stroke_link
          }
      };

      SList_update_entry(&manager->active_strokes_list, &updated_entry);
    }
  } else {
    printf("There is no active entry in SM. Do nothing?\n");
  }
}

// returns NULL if it finished successfuly
// returns UncommittedStroke* if help is needed from the USQ to create an
// active entry from that pointer
UncommittedStroke* SM_redo_stroke(StrokeManager* manager, uint16_t unique_player_index) {
  SEntry* entry = SList_find_entry(&manager->active_strokes_list, unique_player_index);
  printf("The entry is %p\n", entry);
  if (entry != NULL) {
    printf(
        "There is an active entry in SM, going to the next one if it exists, and if not, trying "
        "USQ if it has any UCSs on the path\n"
    );

    // special case when active stroke is all the way at the back of the stroke arr
    if (manager->strokes[entry->data.stroke_index].previous_stroke_index == UINT16_MAX) {
      if (manager->strokes[entry->data.stroke_index].undo == true) {
        manager->strokes[entry->data.stroke_index].undo = false;
        return NULL;
      }
    }

    if (manager->strokes[entry->data.stroke_index].next_stroke_index == UINT16_MAX) {
      // SList_remove_entry(&manager->active_strokes_list, unique_player_index);
      // Try the USQ
      if (entry->data.uncommitted_stroke_link != NULL) {
        // ... create an active UCS entry from that pointer
        return entry->data.uncommitted_stroke_link;
      } else {
        // ...
      }
    } else {
      SEntry updated_entry = (SEntry){
          .unique_player_index = unique_player_index,
          .data = (StrokeContainer){
              .stroke_index = manager->strokes[entry->data.stroke_index].next_stroke_index,
              .uncommitted_stroke_link = entry->data.uncommitted_stroke_link
          }
      };

      SList_update_entry(&manager->active_strokes_list, &updated_entry);

      manager->strokes[entry->data.stroke_index].undo = false;
    }
  } else {
    // A redo here shouldn't have been called in the first place if there were not an active entry
    // in SM
    printf("There is no active entry in SM. Do nothing? (IMPOSSIBILITY)\n");
  }

  return NULL;
}

int32_t SM_add_uncommitted_stroke(
    StrokeManager* manager, UncommittedStroke* uncommitted_stroke_ptr
) {
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
  return manager->strokes_len - 1;
}

void SM_point_active_stroke_to(
    StrokeManager* manager, UncommittedStroke* new_uncommitted_stroke_ptr,
    int16_t unique_player_index
) {
  SEntry* entry = SList_find_entry(&manager->active_strokes_list, unique_player_index);
  if (entry != NULL) {
    printf("FIRST CASE\n");
    entry->data.uncommitted_stroke_link = new_uncommitted_stroke_ptr;
    manager->strokes[entry->data.stroke_index].next_stroke_index = UINT16_MAX;

    // uhhhh
    // if (manager->strokes[entry->data.stroke_index].previous_stroke_index == UINT16_MAX) {
    //   SList_remove_entry(&manager->active_strokes_list, unique_player_index);
    // }
  } else {
    printf("SECOND CASE\n");
    // there is nothing to point, so cannot do anything about it
    // this problem probably needs to be fixed in the USQ_drain function,
    // on every dequeue the freshly dequeued item needs to point to the current active stroke in USQ

    // turns out there is something to do actually,
    // i will create a fake entry here that points to the UCS next in queue
    SEntry new_entry = (SEntry){
        .unique_player_index = unique_player_index,
        .data = (StrokeContainer){
            .stroke_index = UINT32_MAX, .uncommitted_stroke_link = new_uncommitted_stroke_ptr
        }
    };
    SList_add_entry(&manager->active_strokes_list, &new_entry);
  }
}
