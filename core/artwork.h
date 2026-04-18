#ifndef ARTWORK_H
#define ARTWORK_H

#include <pthread.h>

#include "stroke_manager.h"
#include "types.h"

typedef struct {
  StrokeManager* layers;
//   Player
} Artwork;

// void SM_initalize(StrokeManager* manager, uint32_t initial_points_capacity);
// void SM_destroy(StrokeManager* manager);
// void SM_add_stroke(StrokeManager* manager, const Stroke item);
// void SM_add_point(StrokeManager* manager, uint32_t stroke_index, StrokePoint point);
// bool SM_is_empty(StrokeManager* manager);
// void SM_undo_stroke(StrokeManager* manager, int16_t player_index);

#endif
