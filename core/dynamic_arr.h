#ifndef DYNAMIC_ARR_H
#define DYNAMIC_ARR_H

#include <stdbool.h>
#include <stdlib.h>

#include "types.h"

// typedef struct {
//   // int      unique_player_index;
//   // Data    *uncommitted_stroke_ptr;
//   int16_t unique_player_index;
//   UncommittedStroke* uncommitted_stroke_ptr;
// } Entry;

// typedef struct {
//   Entry* items;
//   int count;
//   int capacity;
// } EntryList;

#define DECLARE_LIST(Name, PayloadType)                                                \
  typedef struct {                                                                     \
    uint16_t unique_player_index;                                                       \
    PayloadType data;                                                                  \
  } Name##Entry;                                                                       \
                                                                                       \
  typedef struct {                                                                     \
    Name##Entry* items;                                                                \
    int count;                                                                         \
    int capacity;                                                                      \
  } Name##List;                                                                        \
                                                                                       \
  void Name##List_init(Name##List* l) {                                                \
    l->capacity = 8;                                                                   \
    l->count = 0;                                                                      \
    l->items = malloc(l->capacity * sizeof(Name##Entry));                              \
  }                                                                                    \
                                                                                       \
  void Name##List_add_entry(Name##List* l, Name##Entry* entry_ptr) {                   \
    if (l->count == l->capacity) {                                                     \
      l->capacity *= 2;                                                                \
      l->items = realloc(l->items, l->capacity * sizeof(Name##Entry));                 \
    }                                                                                  \
    l->items[l->count++] = *entry_ptr;                                                 \
  }                                                                                    \
                                                                                       \
  void Name##List_remove_entry(Name##List* l, int unique_player_index) {               \
    for (int i = 0; i < l->count; i++) {                                               \
      if (l->items[i].unique_player_index == unique_player_index) {                    \
        l->items[i] = l->items[--l->count];                                            \
        return;                                                                        \
      }                                                                                \
    }                                                                                  \
  }                                                                                    \
                                                                                       \
  void Name##List_update_entry(Name##List* l, Name##Entry* entry_ptr) {                \
    for (int i = 0; i < l->count; i++) {                                               \
      if (l->items[i].unique_player_index == entry_ptr->unique_player_index) {         \
        l->items[i] = *entry_ptr;                                                      \
        return;                                                                        \
      }                                                                                \
    }                                                                                  \
  }                                                                                    \
                                                                                       \
  Name##Entry* Name##List_find_entry(Name##List* l, int unique_player_index) {         \
    for (int i = 0; i < l->count; i++) {                                               \
      if (l->items[i].unique_player_index == unique_player_index) return &l->items[i]; \
    }                                                                                  \
    return NULL;                                                                       \
  }                                                                                    \
                                                                                       \
  bool Name##List_contains_entry(Name##List* l, int unique_player_index) {             \
    for (int i = 0; i < l->count; i++)                                                 \
      if (l->items[i].unique_player_index == unique_player_index) return true;         \
    return false;                                                                      \
  }

typedef struct {
  UncommittedStroke* uncommitted_stroke_ptr;
} UncommittedStrokeContainer;

typedef struct {
  uint32_t stroke_index;
  UncommittedStroke* uncommitted_stroke_link;
} StrokeContainer;

DECLARE_LIST(UCS, UncommittedStrokeContainer)
DECLARE_LIST(S, StrokeContainer)

// // Iteration
// for (int i = 0; i < l->count; i++) {
//     Entry *e = &l->items[i];
//     // use e->unique_player_index, e->uncommitted_stroke_ptr
// }

#endif
