
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

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

typedef struct Arena {
  u64 capacity;
  u64 pos;
} Arena;

Arena *arena_create(u64 capacity);
void arena_destroy(Arena *a);
void arena_clear(Arena *a);
void *arena_alloc(Arena *a, u64 size);

#endif
