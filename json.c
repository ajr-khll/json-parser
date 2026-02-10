#include "arena.c"

#include <limits.h>
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
  return parser->p++ < parser->end ? (unsigned char)*parser->p : -1;
};

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

JsonValue *parse_value(Parser *parser) {
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

JsonValue *parse_object(Parser *parser) {
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

    // create pair
    JsonPair *pair = (JsonPair *)arena_alloc(parser->arena, sizeof(JsonPair));
    if (!pair) {
      return NULL;
    }

    pair->key = key;
    pair->value = value;

    // link the list
    if (!obj->v.object) {
      obj->v.object = pair;
    } else {
      tail->next = pair;
    }
    tail = pair;
  }

  skip_whitespace(parser);

  int c = consume_char(parser);
  if (c == '}') {
    return obj;
  }
  if (c != ',') {
    return NULL;
  }
}

JsonValue *parse_array(Parser *parser) {
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
    JsonNode *arr_node =
        (JsonNode *)arena_alloc(parser->arena, sizeof(JsonNode));

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

JsonValue *parse_number(Parser *parser) {}
