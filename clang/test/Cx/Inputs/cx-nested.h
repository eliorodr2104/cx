#ifndef CX_NESTED_H
#define CX_NESTED_H

#module Owner

typedef struct Cell {
  int v;
  ~mutating int get() { return v; }
  void bump() { v++; }
} Cell;

typedef struct Holder {
  private struct { int hidden; };
  struct { private int inner; };
  private(set) int arr[2];
  private(set) Cell cell;
  private(set) Cell cells[2];
  private(set) const int fixed[2];
} Holder;

typedef struct Guarded {
  int shown;
  private int secret = 7;
} Guarded;

#endif
