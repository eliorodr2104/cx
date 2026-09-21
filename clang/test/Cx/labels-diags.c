// A declared label is required at direct calls, and an undeclared one is not
// silently accepted (M3).

// RUN: %clang -x cx -std=gnu17 -fsyntax-only -Xclang -verify %s

#module UI

void resize(int width newWidth, // expected-note 2 {{parameter declared here}}
            int height newHeight); // expected-note {{parameter declared here}}
void positional(int w, int h); // expected-note {{parameter declared here}}

void (*fp)(int, int);

void use(void) {
  resize(800, 600); // expected-error {{missing argument label 'width:' in call}} \
                    // expected-error {{missing argument label 'height:' in call}}
  resize(w: 800, height: 600); // expected-error {{incorrect argument label in call (have 'w:', expected 'width:')}}
  positional(width: 1, 2); // expected-error {{extraneous argument label 'width:' in call}}
  fp(width: 1, 2); // expected-error {{argument labels are not available through a function pointer}}
}
