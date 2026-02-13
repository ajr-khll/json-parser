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
  if (!v) {
    return NULL;
  }

  v->type = type;
  memset(&v->v, 0, sizeof(v->v));

  return v;
}

JsonValue *start_json_parse(Arena *a, const char *src, size_t len) {
  Parser parser = {src + len, src, a};
  skip_whitespace(&parser);
  JsonValue *root =
      parse_value(&parser); // gonna be our recursive function which takes us to
                            // the end of the file (barring trailing whitespace)
  skip_whitespace(&parser);
  return root;
};

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

  if (consume_char(parser) != '{')
    return NULL;

  JsonValue *obj = json_new_value(parser, JSON_OBJECT);
  obj->v.object = NULL;
  JsonPair *tail = NULL; // tail of the linked list
  if (get_char(parser) == '}') {
    consume_char(parser);
    return obj; // empty object
  }

  while (1) {
    skip_whitespace(parser);

    if (get_char(parser) != '"') {
      return NULL;
    }

    char *key = parse_string(parser);
    if (!key) {
      return NULL;
    }

    skip_whitespace(parser);

    if (consume_char(parser) != ':') {
      return NULL;
    }

    JsonValue *value = parse_value(parser);

    if (!value) {
      return NULL;
    }

    // create pair
    JsonPair *pair = (JsonPair *)arena_alloc(parser->arena, sizeof(JsonPair));
    if (!pair) {
      return NULL;
    }

    pair->key = key;
    pair->value = value;
    pair->next = NULL;

    // link the list
    if (!obj->v.object) {
      obj->v.object = pair;
    } else {
      tail->next = pair;
    }
    tail = pair;

    skip_whitespace(parser);

    int c = consume_char(parser);
    skip_whitespace(parser);
    if (c == '}') {
      return obj;
    }
    if (c != ',') {
      return NULL;
    }
  }
}

static JsonValue *parse_array(Parser *parser) {
  if (consume_char(parser) != '[') {
    return NULL;
  }

  JsonValue *arr = json_new_value(parser, JSON_ARRAY);
  if (!arr)
    return NULL;

  arr->v.array = NULL;
  JsonNode *tail = NULL; // tail of the linked list

  skip_whitespace(parser);

  if (get_char(parser) == ']') {
    consume_char(parser);
    return arr; // empty array
  }

  while (1) {
    skip_whitespace(parser);

    // create a node
    JsonValue *value = parse_value(parser);

    if (!value)
      return NULL;
    JsonNode *arr_node =
        (JsonNode *)arena_alloc(parser->arena, sizeof(JsonNode));

    if (!arr_node)
      return NULL;
    arr_node->val = value;
    arr_node->next = NULL;

    if (!arr->v.array) {
      arr->v.array = arr_node;
    } else {
      tail->next = arr_node;
    }
    tail = arr_node;

    skip_whitespace(parser);

    int c = consume_char(parser);
    if (c == ']') {
      return arr;
    }
    if (c != ',') {
      return NULL;
    }
  }
}

static JsonValue *parse_number(Parser *parser) {
  const char *start = parser->p;
  const char *p = parser->p;
  if (p >= parser->end) {
    return NULL;
  }

  if (*p == '0') {
    p++;
    // check if leading 0 has digit after it
    if (p < parser->end && *p >= '0' && *p <= '9') {
      return NULL;
    }
  } else if (*p >= '1' && *p <= '9') {
    while (p < parser->end && *p >= '0' && *p <= '9') {
      p++;
    }
  } else {
    return NULL;
  }

  if (p < parser->end && *p == '.') {
    p++;
    if (p >= parser->end || !(*p >= '0' && *p <= '9')) {
      return NULL;
    }
    // must have digits after decimal
    while (p < parser->end && *p >= '0' && *p <= '9') {
      p++;
    }
  }
  if (p < parser->end && (*p == 'e' || *p == 'E')) {
    p++;
    if (p < parser->end && (*p == '+' || *p == '-')) {
      p++;
    }
    if (p >= parser->end || !(*p >= '0' && *p <= '9')) {
      return NULL;
    }
    while (p < parser->end && *p >= '0' && *p <= '9') {
      p++;
    }
  }

  size_t len = (size_t)(p - start);

  char buf[64];

  if (len >= sizeof(buf)) {
    return NULL;
  }

  memcpy(buf, start, len);
  buf[len] = '\0';
  char *endpointer = NULL;

  double value = strtod(buf, &endpointer);
  if (endpointer == buf) {
    return NULL;
  }

  parser->p = p;

  JsonValue *v = json_new_value(parser, JSON_NUMBER);
  if (!v)
    return NULL;
  v->v.number = value;
  return v;
};

static char *parse_string(Parser *parser) {
  char c = consume_char(parser);
  if (!(c == '"')) {
    return NULL;
  }

  const char *start = parser->p;
  const char *p = parser->p;

  while (p < parser->end && *p != '"') {
    p++;
  };

  parser->p = p;
  char buf[64];

  size_t len = (size_t)(p - start);
  if (len >= sizeof(buf)) {
    return NULL;
  }

  memcpy(buf, start, len);
  buf[len] = '\0';

  char *v = (char *)arena_alloc(parser->arena, len + 1);
  memcpy(v, &buf[0], len);

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
