// The type's own implementation reaches every member.
#module Users
#include "cx-user.h"

struct User {
  ~mutating int readAll() { return token + id + generation + cacheVersion; }

  void bump() {
    self.id += 1;
    self.token = 1;
    int *p = &self.id;   // the type may hand out a mutable pointer to itself
    (void)p;
  }

  // Introduced only here, so private by default.
  void helper() { self.token = 2; }
};
