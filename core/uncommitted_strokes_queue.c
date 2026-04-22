#include "uncommitted_strokes_queue.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_arr.h"
#include "stroke_manager.h"
#include "types.h"

void USQ_initalize(UncommittedStrokesQueue* q, uint16_t capacity) {
  pthread_mutex_init(&q->mutex, NULL);
  pthread_cond_init(&q->cond, NULL);

  UCSList_init(&q->active_strokes_list);

  q->begin = 0;
  q->len = 0;

  q->array = malloc(capacity * sizeof(UncommittedStroke*));
  if (q->array == NULL) {
    printf("UncommittedStrokesQueue malloc failed!\n");
    exit(1);
  } else printf("Allocated q of total array size %ld bytes\n", capacity * sizeof(MoveAction));

  q->capacity = capacity;
}

int USQ_find_index(UncommittedStrokesQueue* q, UncommittedStroke* target) {
  for (int i = 0; i < q->len; i++) {
    UncommittedStroke* curr = q->array[(q->begin + i) % q->capacity];
    if (target == curr) return i;
  }
  return -1;
}

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

void USQ_print_refs(
    UncommittedStrokesQueue* q, StrokeManager* manager, int16_t unique_player_index
) {
  printf("SM\n");
  for (uint32_t i = 0; i < manager->strokes_len; i++) {
    Stroke curr = manager->strokes[i];

    SEntry* entry = SList_find_entry(&manager->active_strokes_list, unique_player_index);
    bool active = entry != NULL && i == entry->data.stroke_index;

    if (active) printf(GREEN);

    // for printing the player index
    // printf("[%d] ", unique_player_index);

    if (curr.previous_stroke_index == UINT16_MAX) printf("NULL <- ");
    else printf("%d <- ", curr.previous_stroke_index);

    printf("%d", i);

    if (curr.next_stroke_index == UINT16_MAX) printf(" -> NULL");
    else printf(" -> %d", curr.next_stroke_index);

    if (active) printf(RESET);

    printf("\n");
  }

  printf("USQ\n");
  for (int i = 0; i < q->len; i++) {
    UncommittedStroke* curr = q->array[(q->begin + i) % q->capacity];

    UCSEntry* entry = UCSList_find_entry(&q->active_strokes_list, unique_player_index);
    bool active = entry != NULL && curr == entry->data.uncommitted_stroke_ptr;

    if (active) printf(GREEN);

    // for printing the player index
    // printf("[%d] ", unique_player_index);

    if (curr->prev == NULL) printf("NULL <- ");
    else printf("%d <- ", USQ_find_index(q, curr->prev));

    printf("%d", i);

    if (curr->next == NULL) printf(" -> NULL");
    else printf(" -> %d", USQ_find_index(q, curr->next));

    if (active) printf(RESET);

    printf("\n");
  }
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
  while (q->len > 0 && q->array[q->begin]->finished == true) {
    printf("Freeing a stroke\n");
    //  dequeue process

    UncommittedStroke* uncommitted_stroke_ptr = q->array[q->begin];

    q->begin = (uint16_t)(q->begin + 1) % q->capacity;
    q->len--;

    uint16_t unique_player_index = uncommitted_stroke_ptr->unique_player_index;
    UCSEntry* entry_ptr = UCSList_find_entry(&q->active_strokes_list, unique_player_index);

    // if (entry_ptr == NULL) {
    // if NOT the active UC (uncommitted) stroke for the target player then:
    // the active UC stroke is in the future OR
    // the active UC stroke was in the past OR
    // this UC stroke is out of the natural undo/redo order

    // in all 3 cases i think it should still go to the SM

    // when we're draining, we just want to move stuf from USQ to SM, nothing more to it

    // no active stroke in USQ
    // it may or may not exist in SM

    // find the active entry in SM for the target player index
    SEntry* SM_active_stroke_entry_ptr =
        SList_find_entry(&stroke_manager->active_strokes_list, unique_player_index);

    // if an active stroke exists in SM
    if (SM_active_stroke_entry_ptr != NULL) {
      printf("ON THE RIGHT PATH\n");
      // compare if its pointing to the same one we are pointing to right now
      if (SM_active_stroke_entry_ptr->data.uncommitted_stroke_link == uncommitted_stroke_ptr) {
        // we are 100% sure that the UCS we are currently trying to drain is within
        // the valid undo/redo path and that it's next up

        // we now need to add it to SM and update the SM active
        // entry to point to the next UCS in the undo/redo path

        // add the stroke
        if (uncommitted_stroke_ptr->stroke.previous_stroke_index != UINT16_MAX) {
          uncommitted_stroke_ptr->stroke.previous_stroke_index =
              SM_active_stroke_entry_ptr->data.stroke_index;
        }

        int32_t index = SM_add_uncommitted_stroke(
            stroke_manager, uncommitted_stroke_ptr
        );  // add a flag here as to whether or not it is active??

        // make the current active stroke index (wont be the case soon) point to the next newly
        // added stroke
        if (SM_active_stroke_entry_ptr->data.stroke_index != UINT32_MAX) {
          stroke_manager->strokes[SM_active_stroke_entry_ptr->data.stroke_index].next_stroke_index =
              index;
        }

        // update the active entry to point to the next uncommitted stroke in the undo/redo path
        SEntry updated_SM_entry = (SEntry){
            .unique_player_index = unique_player_index,
            .data = (StrokeContainer){
                .stroke_index = index, .uncommitted_stroke_link = uncommitted_stroke_ptr->next
            }
        };
        SList_update_entry(&stroke_manager->active_strokes_list, &updated_SM_entry);
      } else {
        printf("Letting it drain...\n");
        // this UCS pointer we are trying to drain right now is an entry belonging to the same
        // player yet is not pointed to by the active stroke entry in SM, this means it's 100%
        // invalid and can be safely deleted without even adding it to SM
      }
    } else {
      // if an active stroke entry does not exist in SM and there is no active stroke in USQ
      // idk how this could be produced naturally
      // this is likely an impossibility
      // for now print it with a big visible warning
      printf("WOWWWOWWWWOWWWOWWW REACHED AN IMPOSSIBILIT?\n");

      // change of code resulted in a change of behavior, what's actually happening in
      // this conditional branch is simply where there is simply an absence of
      // the active stroke entry in SM is the following:
      // Either: 1. there is an active stroke in USQ
      // 2. There is no active stroke in USQ (impossible?)
    }

    // alternative: add/update the stroke manager entry right here
    // step1: find the current active stroke if any
    // step2: update the prev_stroke_index of the currently dequeued stroke to point to the
    // index
    // of the current active stroke
    // step3: add/update (depending on whether a current active stroke existed) the entry to
    // {
    //   unique_player_index: unique_player_index
    //   stroke_index: index
    //   uncommitted_stroke_link: NULL
    // }
    // } else {
    //   // an active UCS exists for this player, it needs to be destroyed? i think

    //   SM_add_uncommitted_stroke(
    //       stroke_manager, uncommitted_stroke_ptr
    //   );  // add a flag here as to whether or not it is active??
    //   // passing true to the flag in the prev function should internally mark it as the new
    //   // active
    //   // stroke (stroke manager keeps track of that)
    // }

    if (entry_ptr != NULL) { UCSList_remove_entry(&q->active_strokes_list, unique_player_index); }

    free(uncommitted_stroke_ptr->points);
    free(uncommitted_stroke_ptr);
  }
}

UncommittedStroke* USQ_enqueue(
    UncommittedStrokesQueue* q, Stroke stroke, int16_t unique_player_index, StrokeManager* manager
) {
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

  // keep track of the new stroke as the current active one

  // TODO: it needs to link to the previos active stroke ..
  // and have the previous stroke link to it
  UCSEntry* old_entry_ptr = UCSList_find_entry(&q->active_strokes_list, unique_player_index);

  UCSEntry new_entry = (UCSEntry){
      .unique_player_index = unique_player_index,
      .data = (UncommittedStrokeContainer){.uncommitted_stroke_ptr = new_uncommitted_stroke_ptr}
  };

  if (old_entry_ptr == NULL) {
    printf("old_entry_ptr == NULL\n");
    UCSList_add_entry(&q->active_strokes_list, &new_entry);
    new_uncommitted_stroke_ptr->prev = NULL;
    new_uncommitted_stroke_ptr->next = NULL;

    SM_point_active_stroke_to(manager, new_uncommitted_stroke_ptr, unique_player_index);

    // SEntry* SM_entry = SList_find_entry(&manager->active_strokes_list, unique_player_index);
    // if (SM_entry != NULL) {
    //   printf("Found an entry.........\n");
    //   if (SM_entry->data.stroke_index != UINT32_MAX) {
    //     printf("It has a garbage index.........\n");
    //     if (manager->strokes[SM_entry->data.stroke_index].undo == true) {
    //       printf("The undo is true... REMOVING.........\n");
    //       SList_remove_entry(&manager->active_strokes_list, unique_player_index);
    //     }
    //   }
    // }
  } else {
    printf("old_entry_ptr != NULL\n ");
    old_entry_ptr->data.uncommitted_stroke_ptr->next = new_uncommitted_stroke_ptr;
    printf("step1\n ");
    new_uncommitted_stroke_ptr->prev = old_entry_ptr->data.uncommitted_stroke_ptr;
    printf("step2\n ");
    new_uncommitted_stroke_ptr->next = NULL;
    printf("step3\n ");
    UCSList_update_entry(&q->active_strokes_list, &new_entry);
    printf("step4\n ");
  }
  new_uncommitted_stroke_ptr->unique_player_index = unique_player_index;
  printf("step5\n ");

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

void USQ_undo_stroke(
    UncommittedStrokesQueue* q, StrokeManager* manager, uint16_t unique_player_index
) {
  UCSEntry* active_uncommitted_stroke_entry =
      UCSList_find_entry(&q->active_strokes_list, unique_player_index);

  if (active_uncommitted_stroke_entry != NULL &&
      active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->stroke.undo == false) {
    active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->stroke.undo = true;

    printf("Found an active stroke\n");
    if (active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->prev == NULL) {
      printf("It's the last one, removing the entry completely\n");
      printf("Nvm, leaving it as undo=true\n");

      // commented this out
      // UCSList_remove_entry(&q->active_strokes_list, unique_player_index);

      // // this next function may not be needed as the last active stroke in SM should be
      // automatically assumed to
      // // be the active stroke once the entry for uncommitted strokes is removed
      // SM_mark_active_stroke(manager, unique_player_index);

    } else {
      printf("Pointing it to the one previous to it\n");
      UCSEntry updated_entry = {
          .unique_player_index = unique_player_index,
          .data = (UncommittedStrokeContainer){
              .uncommitted_stroke_ptr =
                  active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->prev
          }
      };
      UCSList_update_entry(&q->active_strokes_list, &updated_entry);
    }
  } else {
    printf("Found no entries in UCS, resorting to the stroke manager\n");
    // If no undoable uncommitted stroke found — fall back to committed
    SM_undo_stroke(manager, unique_player_index);
  }

  USQ_print_refs(q, manager, unique_player_index);
}

void USQ_redo_stroke(
    UncommittedStrokesQueue* q, StrokeManager* manager, uint16_t unique_player_index
) {
  UCSEntry* active_uncommitted_stroke_entry =
      UCSList_find_entry(&q->active_strokes_list, unique_player_index);

  if (active_uncommitted_stroke_entry != NULL) {
    printf("Found an active stroke, maybe undo=true or undo=false\n");

    // special case when active stroke is all the way at the back of the stroke arr
    if (active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->prev == NULL) {
      if (active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->stroke.undo == true) {
        printf("Turns out undo=true... so setting undo=false and stopping\n");
        active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->stroke.undo = false;
        return;
      }
    }

    printf("It appears undo=false... so continuing as usual\n");

    // must check if there is anything next it
    if (active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->next != NULL) {
      printf("Pointing it to the one next to it\n");

      active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->next->stroke.undo = false;
      UCSEntry updated_entry = {
          .unique_player_index = unique_player_index,
          .data = (UncommittedStrokeContainer){
              .uncommitted_stroke_ptr =
                  active_uncommitted_stroke_entry->data.uncommitted_stroke_ptr->next
          }
      };
      UCSList_update_entry(&q->active_strokes_list, &updated_entry);

    } else {
      // nothing to do here
    }
  } else {
    // first check that
    printf("Found no entries in UCS, resorting to the stroke manager\n");
    // If no undoable uncommitted stroke found — fall back to committed
    UncommittedStroke* response = SM_redo_stroke(manager, unique_player_index);
    if (response == NULL) {
      // finished successfully
    } else {
      response->stroke.undo = false;
      UCSEntry new_entry = {
          .unique_player_index = unique_player_index,
          .data = (UncommittedStrokeContainer){.uncommitted_stroke_ptr = response}
      };
      UCSList_add_entry(&q->active_strokes_list, &new_entry);
    }
  }

  USQ_print_refs(q, manager, unique_player_index);
}
