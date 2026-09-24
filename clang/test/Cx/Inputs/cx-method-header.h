#ifndef CX_METHOD_HEADER_H
#define CX_METHOD_HEADER_H

#module Geo

typedef struct Pt {
  int x;
  int get() { return x; }
  void set(int v);
} Pt;

#endif
