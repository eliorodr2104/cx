// In an unowned file there is one C symbol per name, so two declarations that
// disagree about argument labels are the same entity and must agree (M3).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

void differ(int width newWidth); // expected-note {{parameter declared here}}
void differ(int w) { (void)w; } // expected-error {{argument labels in redeclaration of 'differ' do not match the previous declaration}}

void agree(int width newWidth);
void agree(int width w) { (void)w; } // local names may differ
