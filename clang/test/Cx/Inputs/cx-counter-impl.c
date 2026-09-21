// The continuation that implements Counter, in its own translation unit.
#module Counters
#include "cx-counter.h"

struct Counter {
  void increment() { self.value += 1; }

  ~mutating int current() { return value; }

  // A helper introduced only here.
  void resetLocalState() { self.value = 0; }
};
