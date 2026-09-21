// Whether two declarations that disagree about labels are one entity or two
// depends on whether they can have two symbols (M3).

// An unowned file has one C symbol per name, so a redeclaration must keep the
// labels its previous declaration established. Local names may differ.
// RUN: %clang -x c -std=gnu17 -fsyntax-only -Xclang -verify %s
// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

void agree(int width, int height);
void agree(int w, int h) { (void)w; (void)h; }

#ifdef LABELED
void differ(int width newWidth); // expected-note {{parameter declared here}}
void differ(int w) { (void)w; } // expected-error {{argument labels in redeclaration of 'differ' do not match the previous declaration}}
#endif
// expected-no-diagnostics
