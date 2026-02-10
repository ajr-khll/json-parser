#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)
#define ARENA_OFFSET (sizeof(Arena))
#define ARENA_ALIGNMENT (sizeof(void *))

#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#define MAX(a, b) (((a) > (b) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))

typedef struct {
  u64 capacity;
  u64 pos;
} Arena;

Arena *arena_create(u64 capacity) {
  Arena *arena = (Arena *)malloc(capacity);
  assert(arena);
  arena->capacity = capacity;
  arena->pos = ARENA_OFFSET;
  return arena;
};

void arena_destroy(Arena *a) { free(a); };

void *arena_alloc(Arena *a, u64 size) {
  // check if there's enough space in the arena?
  u64 aligned_pos = ALIGN_UP_POW2(a->pos, ARENA_ALIGNMENT);
  if ((aligned_pos + size) > a->capacity) {
    return NULL;
  };

  void *result = aligned_pos + (u8 *)a;
  a->pos = aligned_pos + size;

  return result;
};

void arena_pop(Arena *a, u64 size) {
  // rollback arena cursor by size bytes, or to 0
  assert(a->pos >= ARENA_OFFSET);
  u64 max_pop = a->pos - ARENA_OFFSET;
  u64 actual_pop =
      MIN(size, max_pop); // either we remove the exact amount, or we
                          // remove the entire contents of the arena
  a->pos -= actual_pop;
}

void arena_pop_to(Arena *a, u64 pos) {
  assert(pos <= a->pos);
  assert(pos >= ARENA_OFFSET);

  arena_pop(a, a->pos - pos);
}

void arena_clear(Arena *a) { arena_pop_to(a, ARENA_OFFSET); }

u64 arena_mark(Arena *a) { return a->pos; }
