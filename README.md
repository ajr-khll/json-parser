# JSON Arena Parser

A tiny, recursive-descent JSON parser written in C. It uses an **Arena Allocator** for memory management, making it fast, simple, and leak-free. No walking the tree to `free()` individual nodes—just destroy the arena and move on.

## Features

* **Arena-Backed:** Zero fragmentation and lightning-fast allocations.
* **Simple API:** Direct file-to-tree parsing.
* **Linked-List Nodes:** Objects and arrays are stored as simple linked lists for easy iteration.
* **Clean Exit:** One call to `arena_destroy` wipes the entire tree.

## Usage

### 1. Compile & Run

The parser expects a JSON file as the first argument.

```bash
cc main.c json.c arena.c read.c -o jparse
./jparse data.json

```

### 2. Integration

```c
// Create a 1MB arena
Arena *arena = arena_create(1024 * 1024);

// Load and parse
char *json_text = NULL;
u64 file_size = 0;
read_file_arena(arena, "file.json", &json_text, &file_size);

JsonValue *root = start_json_parse(arena, json_text, file_size);

// Print or traverse the tree
if (root) {
    print_json(root, 0);
}

// Global cleanup
arena_destroy(arena);

```

## Tree Structure

The parser generates a `JsonValue` tree:

* **JSON_OBJECT:** Linked list of `JsonPair` (string key + `JsonValue`).
* **JSON_ARRAY:** Linked list of `JsonNode` (`JsonValue` pointers).
* **JSON_NUMBER:** Stored as `double`.
* **JSON_STRING:** Null-terminated `char*` allocated in the arena.

## Limitations

* **Strict JSON:** Does not support trailing commas.
* **Escapes:** Does not currently process escape sequences (e.g., `\uXXXX` or `\n`).
* **Encoding:** Optimized for UTF-8.
