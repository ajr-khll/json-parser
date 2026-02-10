#ifndef READ_H
#define READ_H
#include "arena.c"
#include <stdint.h>

b32 read_file_arena(Arena *a, const char *path, char **out_data, u64 *out_size);

#endif
