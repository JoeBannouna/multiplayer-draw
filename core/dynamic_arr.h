#ifndef DYNAMIC_ARR_H
#define DYNAMIC_ARR_H

#include <stdbool.h>
#include <stdlib.h>

#include "types.h"

typedef struct {
  // int      unique_player_index;
  int16_t unique_player_index;
  UncommittedStroke* uncommitted_stroke_ptr;
  // Data    *uncommitted_stroke_ptr;
} Entry;

typedef struct {
  Entry* items;
  int count;
  int capacity;
} EntryList;

void list_init(EntryList* l) {
  l->capacity = 8;
  l->count = 0;
  l->items = malloc(l->capacity * sizeof(Entry));
}

void list_add(EntryList* l, int idx, UncommittedStroke* ptr) {
  if (l->count == l->capacity) {
    l->capacity *= 2;
    l->items = realloc(l->items, l->capacity * sizeof(Entry));
  }
  l->items[l->count++] = (Entry){.unique_player_index = idx, .uncommitted_stroke_ptr = ptr};
}

// Swap-and-pop removal — O(n), does not preserve order
void list_remove(EntryList* l, int unique_player_index) {
  for (int i = 0; i < l->count; i++) {
    if (l->items[i].unique_player_index == unique_player_index) {
      l->items[i] = l->items[--l->count];  // overwrite with last
      return;
    }
  }
}

void list_replace(EntryList* l, int unique_player_index, UncommittedStroke* new_active_stroke) {
  for (int i = 0; i < l->count; i++) {
    if (l->items[i].unique_player_index == unique_player_index) {
      l->items[i] = (Entry){
          .unique_player_index = unique_player_index, .uncommitted_stroke_ptr = new_active_stroke
      };
      return;
    }
  }
}

// Find — O(n)
Entry* list_find(EntryList* l, int unique_player_index) {
  for (int i = 0; i < l->count; i++) {
    if (l->items[i].unique_player_index == unique_player_index) return &l->items[i];
  }
  return NULL;
}

// Existence check — O(n)
bool list_contains(EntryList* l, int unique_player_index) {
  for (int i = 0; i < l->count; i++)
    if (l->items[i].unique_player_index == unique_player_index) return true;
  return false;
}

// // Iteration
// for (int i = 0; i < l->count; i++) {
//     Entry *e = &l->items[i];
//     // use e->unique_player_index, e->uncommitted_stroke_ptr
// }

#endif
