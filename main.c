#include "json.h"
#include "read.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s file.json\n", argv[0]);
    return 1;
  }

  Arena *arena = arena_create(1024 * 1024);
  if (!arena) {
    printf("Failed to create arena\n");
    return 1;
  }

  char *json_text = NULL;
  u64 file_size = 0;

  if (!read_file_arena(arena, argv[1], &json_text, &file_size)) {
    printf("Failed to read file\n");
    return 1;
  }

  JsonValue *root = start_json_parse(arena, json_text, file_size);
  if (!root) {
    printf("Parse error\n");
    arena_destroy(arena);
    return 1;
  }

  print_json(root, 0);
  printf("\n");

  arena_destroy(arena);
  return 0;
}
