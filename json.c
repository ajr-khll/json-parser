#include "json.h"
#include "arena.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static JsonValue *parse_value(Parser *parser);
static JsonValue *parse_object(Parser *parser);
static JsonValue *parse_array(Parser *parser);
static char *parse_string(Parser *parser);
static JsonValue *parse_number(Parser *parser);
static JsonValue *parse_true(Parser *parser);
static JsonValue *parse_false(Parser *parser);
static JsonValue *parse_null(Parser *parser);
static JsonValue *json_new_value(Parser *parser,
                                 JsonType type); // constructor for JsonValue
static inline int get_char(Parser *parser);
static inline int consume_char(Parser *parser);

static inline int get_char(Parser *parser) {
  return parser->p < parser->end ? (unsigned char)*parser->p : -1;
}

static inline int consume_char(Parser *parser) {
  if (parser->p >= parser->end)
    return -1;
  return (unsigned char)*parser->p++;
}

static void skip_whitespace(Parser *parser) {
  while (parser->p < parser->end) {
    char curr = *parser->p;
    if (curr == ' ' || curr == '\t' || curr == '\n' ||
        curr == '\r') // skip all the whitespace lol
      parser->p++;
    else
      break;
  }
}

static JsonValue *json_new_value(Parser *parser, JsonType type) {
  JsonValue *v = (JsonValue *)arena_alloc(parser->arena, sizeof(JsonValue));
  if (!v)
    return NULL;
  v->type = type;
  memset(&v->v, 0, sizeof(v->v));
  return v;
}

JsonValue *start_json_parse(Arena *a, const char *src, size_t len) {
  Parser parser = {src, src + len, a};
  skip_whitespace(&parser);
  JsonValue *root = parse_value(&parser);
  skip_whitespace(&parser);
  return root;
}

static JsonValue *parse_value(Parser *parser) {
  int c = get_char(parser);
  // figure out what value
  switch (c) {
  case '{':
    return parse_object(parser);

  case '[':
    return parse_array(parser);

  case '"': {
    char *s = parse_string(parser);
    if (!s)
      return NULL;
    JsonValue *v = json_new_value(parser, JSON_STRING);
    v->v.string = s;
    return v;
  }

  case 't':
    return parse_true(parser);

  case 'f':
    return parse_false(parser);

  case 'n':
    return parse_null(parser);

  case '-':
    return parse_number(parser);

  default:
    if (c >= '0' && c <= '9') {
      return parse_number(parser);
    }

    return NULL;
  }
};

static JsonValue *parse_object(Parser *parser) {
  if (consume_char(parser) != '{') {
    return NULL;
  }

  JsonValue *obj = json_new_value(parser, JSON_OBJECT);
  if (!obj)
    return NULL;

  obj->v.object = NULL;
  JsonPair *tail = NULL;

  while (1) {
    skip_whitespace(parser);

    // Check for empty object or end of object
    if (get_char(parser) == '}') {
      consume_char(parser);
      return obj;
    }

    // If not '}', we expect a key string
    if (get_char(parser) != '"') {
      return NULL;
    }

    char *key = parse_string(parser);
    if (!key)
      return NULL;

    skip_whitespace(parser);

    // Expect colon
    if (consume_char(parser) != ':')
      return NULL;

    skip_whitespace(parser);

    // Parse the value
    JsonValue *value = parse_value(parser);
    if (!value)
      return NULL;

    // Create the pair
    JsonPair *pair = (JsonPair *)arena_alloc(parser->arena, sizeof(JsonPair));
    if (!pair)
      return NULL;

    pair->key = key;
    pair->value = value;
    pair->next = NULL;

    // Append to linked list
    if (!obj->v.object) {
      obj->v.object = pair;
    } else {
      tail->next = pair;
    }
    tail = pair;

    skip_whitespace(parser);

    int c = get_char(parser);
    if (c == '}') {
      consume_char(parser);
      return obj;
    } else if (c == ',') {
      consume_char(parser);
    } else {
      return NULL;
    }
  }
}
static JsonValue *parse_array(Parser *parser) {
  if (consume_char(parser) != '[')
    return NULL;

  JsonValue *arr = json_new_value(parser, JSON_ARRAY);
  if (!arr)
    return NULL;

  JsonNode *tail = NULL;
  skip_whitespace(parser);

  if (get_char(parser) == ']') {
    consume_char(parser);
    return arr;
  }

  while (1) {
    skip_whitespace(parser);
    JsonValue *value = parse_value(parser);
    if (!value)
      return NULL;

    JsonNode *node = (JsonNode *)arena_alloc(parser->arena, sizeof(JsonNode));
    if (!node)
      return NULL;

    node->val = value;
    node->next = NULL;

    if (!arr->v.array) {
      arr->v.array = node;
    } else {
      tail->next = node;
    }
    tail = node;

    skip_whitespace(parser);
    int c = consume_char(parser);
    if (c == ']')
      return arr;
    if (c != ',')
      return NULL;
  }
}

static char *parse_string(Parser *parser) {
  if (consume_char(parser) != '"')
    return NULL;

  const char *start = parser->p;
  while (parser->p < parser->end && *parser->p != '"') {
    parser->p++;
  }

  if (parser->p >= parser->end)
    return NULL;

  size_t len = (size_t)(parser->p - start);
  char *v = (char *)arena_alloc(parser->arena, len + 1);
  if (!v)
    return NULL;

  memcpy(v, start, len);
  v[len] = '\0';

  consume_char(parser);
  return v;
}

static JsonValue *parse_number(Parser *parser) {
  const char *start = parser->p;

  if (parser->p < parser->end && *parser->p == '-') {
    parser->p++;
  }

  if (parser->p < parser->end && *parser->p == '0') {
    parser->p++;
    if (parser->p < parser->end && *parser->p >= '0' && *parser->p <= '9') {
      return NULL;
    }
  } else if (parser->p < parser->end && *parser->p >= '1' &&
             *parser->p <= '9') {
    while (parser->p < parser->end && *parser->p >= '0' && *parser->p <= '9') {
      parser->p++;
    }
  } else {
    return NULL;
  }

  if (parser->p < parser->end && *parser->p == '.') {
    parser->p++;
    if (parser->p >= parser->end || !(*parser->p >= '0' && *parser->p <= '9')) {
      return NULL;
    }
    while (parser->p < parser->end && *parser->p >= '0' && *parser->p <= '9') {
      parser->p++;
    }
  }

  if (parser->p < parser->end && (*parser->p == 'e' || *parser->p == 'E')) {
    parser->p++;
    if (parser->p < parser->end && (*parser->p == '+' || *parser->p == '-')) {
      parser->p++;
    }
    if (parser->p >= parser->end || !(*parser->p >= '0' && *parser->p <= '9')) {
      return NULL;
    }
    while (parser->p < parser->end && *parser->p >= '0' && *parser->p <= '9') {
      parser->p++;
    }
  }

  char *endptr;
  double value = strtod(start, &endptr);
  if (endptr == start)
    return NULL;

  JsonValue *v = json_new_value(parser, JSON_NUMBER);
  if (!v)
    return NULL;
  v->v.number = value;
  return v;
}
static JsonValue *parse_true(Parser *parser) {
  const char *start = parser->p;

  if (parser->end - start < 4 || start[0] != 't' || start[1] != 'r' ||
      start[2] != 'u' || start[3] != 'e') {
    return NULL;
  }

  parser->p += 4;

  JsonValue *v = json_new_value(parser, JSON_BOOL);
  if (!v)
    return NULL;

  v->v.boolean = 1;
  return v;
}

static JsonValue *parse_false(Parser *parser) {
  const char *start = parser->p;

  if (parser->end - start < 5 || start[0] != 'f' || start[1] != 'a' ||
      start[2] != 'l' || start[3] != 's' || start[4] != 'e') {
    return NULL;
  }

  parser->p += 5;

  JsonValue *v = json_new_value(parser, JSON_BOOL);
  if (!v)
    return NULL;

  v->v.boolean = 0;
  return v;
}

static JsonValue *parse_null(Parser *parser) {
  const char *start = parser->p;

  if (parser->end - start < 4 || start[0] != 'n' || start[1] != 'u' ||
      start[2] != 'l' || start[3] != 'l') {
    return NULL;
  }

  parser->p += 4;

  JsonValue *v = json_new_value(parser, JSON_NULL);
  if (!v)
    return NULL;

  return v;
}

static void print_indent(int n) {
  for (int i = 0; i < n; i++)
    putchar(' ');
}

void print_json(JsonValue *v, int indent) {
  if (!v)
    return;

  switch (v->type) {
  case JSON_NULL:
    printf("null");
    break;

  case JSON_BOOL:
    printf(v->v.boolean ? "true" : "false");
    break;

  case JSON_NUMBER:
    printf("%g", v->v.number);
    break;

  case JSON_STRING:
    printf("\"%s\"", v->v.string);
    break;

  case JSON_ARRAY: {
    printf("[\n");
    JsonNode *node = v->v.array;
    while (node) {
      print_indent(indent + 2);
      print_json(node->val, indent + 2);
      if (node->next)
        printf(",");
      printf("\n");
      node = node->next;
    }
    print_indent(indent);
    printf("]");
    break;
  }

  case JSON_OBJECT: {
    printf("{\n");
    JsonPair *pair = v->v.object;
    while (pair) {
      print_indent(indent + 2);
      printf("\"%s\": ", pair->key);
      print_json(pair->value, indent + 2);
      if (pair->next)
        printf(",");
      printf("\n");
      pair = pair->next;
    }
    print_indent(indent);
    printf("}");
    break;
  }
  }
}
