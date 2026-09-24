#module Beta
int get(int v); // expected-error {{'get' is first declared in module 'Alpha', so module 'Beta' cannot declare it}}
extern int counter; // expected-error {{'counter' is first declared in module 'Alpha', so module 'Beta' cannot declare it}}
int get(double v);
