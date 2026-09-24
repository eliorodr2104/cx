// Inputs that crashed the compiler before M4.4, and the rules that now reject
// them. The code-generation cases are in crash-regressions-codegen.c.

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -ferror-limit=0 -Xclang -verify=expected,gnu %s
// RUN: %clang -x cx -std=c23 -fsyntax-only -ferror-limit=0 -Xclang -verify=expected,c23 %s

#module Crash

// `var` and `let` follow C's rule for braces and arrays, each spelled as
// written.
void braces(void) {
  let c = {1}; // expected-error {{cannot use 'let' with initializer list in C}}
  var d = {}; // expected-error {{cannot use 'var' with initializer list in C}}
  var e[] = {1}; // expected-error {{cannot use 'var' with array in C}}
  let f[2] = 0; // gnu-error {{cannot use 'let' with array in C}} c23-error {{'let' not allowed in array declaration}}
}

struct Bits { int field : 3; } bits;
void bitfield(void) {
  var v = bits.field; // expected-error {{cannot pass bit-field as 'var' initializer in C}}
}

// A method reference is only ever the callee of a call.
struct Counter {
  int value;
  int get() { return value; }
  void touch() {
    get = 0; // expected-error {{reference to Cx method 'get' must be a call}}
    (void)&get; // expected-error {{reference to Cx method 'get' must be a call}}
  }
};

void method_refs(struct Counter *p, struct Counter c) {
  p->get = 0; // expected-error {{reference to Cx method 'get' must be a call}}
  (void)&p->get; // expected-error {{reference to Cx method 'get' must be a call}}
  (void)sizeof(p->get); // expected-error {{reference to Cx method 'get' must be a call}}
  __typeof__(c.get) *q; // expected-error {{reference to Cx method 'get' must be a call}}
  p->get; // expected-error {{reference to Cx method 'get' must be a call}}
  (void)(p->get, 1); // expected-error {{reference to Cx method 'get' must be a call}}
  (void)p->get();
  (void)(c.get)();
}

// A continuation cannot add storage, including through an anonymous member.
struct Grow { int a; }; // expected-note 2 {{primary definition of 'Grow' is here}}
int grow_before = sizeof(struct Grow);
struct Grow {
  union { int b; float f; }; // expected-error {{a continuation of 'Grow' cannot add a stored field}}
};
struct Grow {
  struct { int d; }; // expected-error {{a continuation of 'Grow' cannot add a stored field}}
};
int grow_use(struct Grow *g) { return g->b; } // expected-error {{no member named 'b' in 'struct Grow'}}
_Static_assert(sizeof(struct Grow) == sizeof(int), "layout is fixed");

// A tag redefined in a parameter list is C's new type, not a continuation.
// Before C23 C warns about it; C23 accepts it as a compatible redefinition.
struct Outer { struct Inner { int j; } in; }; // gnu-note {{previous definition is here}}
int proto(struct Outer { struct Inner { int j; } in; } *); // gnu-warning {{declaration of 'struct Outer' will not be visible outside of this function}} gnu-warning {{redefinition of 'Inner' will not be visible outside of this function}}

// A default is evaluated wherever the type is constructed, where the locals of
// the function declaring it do not exist.
int local_default(void) {
  int k = 3; // expected-note {{declared here}}
  static int s = 1;
  struct L { int a = k; }; // expected-error {{a field default cannot refer to 'k', a local variable of the enclosing function}}
  struct M { int a = s; unsigned long b = sizeof(k); int c = ({ int t = 2; t; }); };
  return 0;
}

// The receiver is always `self`.
struct Named {
  int v;
  void set(int self) {} // expected-error {{'self' names the receiver of a Cx method, so a parameter cannot be named 'self'}}
};

// A method and a stored member cannot share a name.
struct Dup1 { int size; int size(); }; // expected-note {{previous declaration is here}} expected-error {{duplicate member 'size'}}
struct Dup2 { int count(); int count; }; // expected-note {{previous declaration is here}} expected-error {{duplicate member 'count'}}
struct Dup3 { int v; }; // expected-note {{previous declaration is here}}
struct Dup3 { int v() { return 0; } }; // expected-error {{duplicate member 'v'}}

// Same parameters and labels are the same method, whatever the return type or
// the receiver's mutability.
struct Conflict {
  void m() {} // expected-note {{previous declaration is here}}
  int m() { return 0; } // expected-error {{conflicting types for 'm'}}
  ~mutating int g(); // expected-note {{previous declaration is here}}
  int g(); // expected-error {{conflicting types for 'g'}}
};

// Member introducers.
struct Intro {
  ~mutating int field; // expected-error {{'~mutating' applies to a method, not a field}}
  int f(), x { return 1; } // expected-error {{a method body must follow a declaration with a single declarator}}
  int after;
  ~mutating private int hidden() { return after; } // expected-note {{private member 'hidden' declared here}}
};
int intro_use(struct Intro *i) { return i->hidden(); } // expected-error {{private member 'hidden' is not permitted here}}
