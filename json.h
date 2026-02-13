
#ifndef JSON_H
#define JSON_H

#include "arena.h"

typedef enum {
  JSON_NULL,
  JSON_BOOL,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT
} JsonType;

typedef struct JsonValue JsonValue;
typedef struct JsonPair JsonPair;
typedef struct JsonNode JsonNode;

struct JsonValue {
  JsonType type;
  union {
    double number;
    b32 boolean;
    char *string;
    JsonPair *object;
    JsonNode *array;
  } v;
};

struct JsonPair {
  char *key;
  JsonValue *value;
  JsonPair *next;
};

struct JsonNode {
  JsonValue *val;
  JsonNode *next;
};

typedef struct Parser {
  const char *end;
  const char *p;
  Arena *arena;
} Parser;

JsonValue *json_parse(Arena *arena, const char *src, size_t len);

void print_json(JsonValue *v, int indent);

JsonValue *start_json_parse(Arena *a, const char *src, size_t len);

#endif
