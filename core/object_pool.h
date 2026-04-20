#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <assert.h>

#define POOL_DEFINE(T, N)                            \
  typedef struct {                                   \
    T buf[N];                                        \
    uint32_t free[N];                                \
    uint32_t free_count;                             \
  } T##Pool;                                         \
                                                     \
  void T##Pool_init(T##Pool* p) {                    \
    p->free_count = N;                               \
    for (uint32_t i = 0; i < N; i++) p->free[i] = i; \
  }                                                  \
                                                     \
  uint32_t T##Pool_alloc(T##Pool* p) {               \
    if (p->free_count == 0) {                        \
      assert(0 && #T "Pool exhausted");              \
      return UINT32_MAX;                             \
    }                                                \
    return p->free[--p->free_count];                 \
  }                                                  \
                                                     \
  void T##Pool_free(T##Pool* p, uint32_t idx) {      \
    assert(idx < N);                                 \
    p->free[p->free_count++] = idx;                  \
  }                                                  \
                                                     \
  T* T##Pool_get(T##Pool* p, uint32_t idx) {         \
    assert(idx < N);                                 \
    return &p->buf[idx];                             \
  }

#endif
