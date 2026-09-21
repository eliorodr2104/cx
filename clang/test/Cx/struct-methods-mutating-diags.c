// Writing through the receiver of a `~mutating` method breaks the promise the
// method made, and the message says so instead of talking about the type of
// `self` (M4.2).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module Shapes

struct Box {
  int a;
  int nested[2];

  ~mutating void writeField() { a = 1; } // expected-error {{cannot modify 'a' through the receiver of '~mutating' method 'writeField'}} \
                                         // expected-note {{remove '~mutating' from 'writeField' if it modifies the receiver}}

  ~mutating void writeElement() { nested[0] = 1; } // expected-error {{cannot modify 'nested' through the receiver of '~mutating' method 'writeElement'}} \
                                                   // expected-note {{remove '~mutating' from 'writeElement' if it modifies the receiver}}

  ~mutating void writeThroughSelf() { self.a += 1; } // expected-error {{cannot modify 'a' through the receiver of '~mutating' method 'writeThroughSelf'}} \
                                                     // expected-note {{remove '~mutating' from 'writeThroughSelf' if it modifies the receiver}}

  // A mutating method may write whatever it likes.
  void allowed() { a = 1; nested[0] = 2; }

  // A local is not the receiver.
  ~mutating int reads() { int copy = a; copy += 1; return copy; }
};
