#module Shared

void release(int id)

typedef struct Handle {
  int id
  init(int i) { id = i }
  deinit() { release(id) }
} Handle

typedef struct Owner {
  Handle h
  int n
} Owner
