#include "arena.h"
#include <stdbool.h>
#include <stdio.h>

b32 read_file_arena(Arena *a, const char *path, char **out_data,
                    u64 *out_size) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    return false;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return false;
  }

  long file_size = ftell(f);
  if (file_size < 0) {
    fclose(f);
    return false;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }

  char *buffer = (char *)arena_alloc(a, (u64)file_size + 1);
  if (!buffer) {
    fclose(f);
    return false;
  }

  size_t read_count = fread(buffer, 1, (size_t)file_size, f);
  fclose(f);

  if (read_count != (size_t)file_size) {
    return false;
  }

  buffer[file_size] = '\0'; // NUL-terminate for convenience

  if (out_data)
    *out_data = buffer;
  if (out_size)
    *out_size = (u64)file_size;

  return true;
}
