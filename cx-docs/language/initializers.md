# Initializers, Defaults, and Destruction

## Construction

Cx uses `init(...)` in a type and `Type(...)` at the call site. No `new` or `alloc`
keyword is required. Struct construction produces a value; class construction
produces an owned class reference.

```c
struct Rect {
    float width
    float height

    init(
        float width  newWidth,
        float height newHeight
    )
}

struct Rect {
    init(
        float width  newWidth,
        float height newHeight
    ) {
        self.width  = newWidth
        self.height = newHeight
    }
}
```

## Automatic construction surface

With no user-declared initializer, Cx synthesizes construction for the stored fields.
Fields without declaration defaults require construction values. Defaulted fields
use their declared defaults; an empty call is available when nothing is required.

```c
struct Size {
    float width
    float height
}

Size size = Size(width: 80, height: 40)
```

Generated field-name labels are compiler-provided construction metadata, not a
requirement to write declarations such as `float width width`. Whether the generated
surface additionally accepts positional arguments is explicitly pending in G02.

A synthesized initializer must not expose inaccessible state or types as a public
construction API. Visibility is bounded by the fields it exposes.

## Custom initializers replace synthesis

Declaring any custom `init`, even before its body is available, suppresses the
memberwise/default initializer synthesis. The custom overload set is the complete
construction interface. Field default expressions continue to participate.

This is the latest decision and supersedes the earlier idea that generated and
custom initializers coexist automatically.

## Initialization order: do not reorder the program

Stored-field layout and automatic default/memberwise initialization follow field
declaration order. A custom initializer body is ordinary code: its statements and
observable effects are not silently reordered to match field order.

The implementation must distinguish first initialization from assignment to an
already initialized field. Defaults execute according to the automatic initialization
phase; overwriting a managed default performs normal assignment cleanup.
Whether a default may refer to an earlier field is still open.

## Definite initialization

Every successful initializer must establish every stored field. Cx must not fill
non-nil class fields with null simply to avoid a diagnostic.

```c
struct Rect {
    float width
    float height

    init(float side) {
        if (side > 0) {
            width = side
            return          // error: 'height' is not initialized
        }
        width = 1
        height = width      // width is initialized on this path
    }
}
```

- **What initializes a field.** A declaration default, or an assignment of the whole
  field, `field = value` or `self.field = value`. A partial write such as
  `origin.x = 0` does not initialize `origin`, and neither does passing `&field` to a
  function; both are errors while the field is uninitialized.
- **Every path.** A field is initialized at a point only if every path reaching it
  initializes it: after `if`/`else` both branches must, and a field assigned only
  inside a loop is not initialized after it. Every exit, `return` or the closing
  brace, must have every field initialized.
- **Reads.** Reading a field that is not yet initialized is an error.
- **Using `self`.** Until every field is initialized, `self` may only have its fields
  assigned or read: calling a method on it, passing `self` or `&self`, and copying
  `*self` are errors.
- **`const` fields.** An initializer may assign a `const` field once on each path;
  that assignment is its initialization. A second assignment is an error.
- **Array and aggregate fields** cannot be assigned whole in C, so they are
  initialized by a default, including a braced one: `int counts[4] = {}`,
  `Point origin = {1, 2}`. Once initialized, their elements are ordinary writes.
- **Anonymous members.** The members of an anonymous struct are fields of their
  own; an anonymous union is initialized once any one of its members is, and any of
  its members may then be read.

### `defer` in an initializer

A deferred block may use `self` and its fields only where they are already
initialized when the `defer` is registered: a field initialized then is still
initialized when the block runs. A deferred block may reassign an initialized field
but cannot initialize one, nor assign a `const` field.

```c
init(char* path) {
    handle = fopen(path, "r")
    defer { log(handle) }       // handle is initialized here
    ...
}
```

## Construction goes through `init`

A value of a type with a custom initializer is made by `Type(...)`, not by braces,
at any depth:

```c
Rect r = { 3, 4 }             // error: Rect has an initializer
Frame f = { { 3, 4 }, 1 }     // error on the Rect member
Frame g = { Rect(3, 4), 1 }   // ok
```

Copying an existing value and declaring a variable without an initializer stay C.
Implicit zero-initialization also stays C: static storage, and the members a braced
initializer leaves out, hold zero without running an initializer.

## Delegation

An initializer may delegate to another initializer of the same type with
`self.init(...)`, which is resolved like any construction of the type.

```c
init(float side) {
    if (side < 0) side = 0
    self.init(width: side, height: side)
    log(self.area())            // self is complete here
}
```

- Every path through a delegating initializer calls `self.init` exactly once.
- Before that call, `self` is not used at all, not even to assign a field; the
  arguments may use parameters and locals.
- After it, `self` is fully initialized.
- Declaration defaults are applied once, by the construction, not again by the
  delegated call.
- `self.init` outside an initializer is an error.

## Throwing initialization

```c
init(String path filePath) throw(FileError)
var file = try File(path: "settings.txt")
```

Failure destroys only the fields that were successfully initialized, in reverse
initialization order, and releases allocated object storage. It does not run the
normal user `deinit()` of an object that never completed construction.

## `deinit`

Automatic field cleanup always exists where needed. A custom `deinit()` adds a hook
before that cleanup; it does not suppress it. `deinit` cannot propagate a Cx error.
A throwing cleanup operation must be handled locally or exposed separately as an
explicit operation before destruction.

```c
struct File {
    FILE* handle

    init(char* path) { handle = fopen(path, "r") }
    deinit() { fclose(handle) }
}
```

- `deinit()` takes no parameters and declares no return type. It is declared in the
  type's primary definition; its body may be in a continuation.
- Destruction runs the body, then destroys every field that has a `deinit`, in
  reverse declaration order. A type with no `deinit` of its own and a field that
  has one receives an implicit one.
- A type with a `deinit`, of its own or through a field, is a *resource type*.

### Resource types do not copy

A resource value has one owner. Copying an existing value, `b = a`, `use(a)`,
`return a` or reading `*p`, is an error: two copies would release the same
resource twice. A new value, a construction or a call's result, moves to where it
is stored. A type that can be duplicated says so with an ordinary method that
returns a new value.

- **Locals** are initialized where they are declared and destroyed when their
  scope is left, on the same cleanup stack as `defer`. A jump past a resource
  local's declaration is an error.
- **Fields** are destroyed with the value that holds them. In an initializer a
  resource field is initialized once on each path, like a `const` field.
- **Assignment** `f = File("b.txt")` builds the new value, destroys the old one,
  then stores the new one.
- **A discarded value**, `File("x")` alone, `(void)open()` or the left operand of a
  comma, is destroyed at once. Reading a field of a new value, `open().handle`,
  is an error.
- **Arrays** are initialized with every element, `File fs[2] = { File("a"),
  File("b") }`, and destroyed from the last element to the first.
- Parameters, variadic arguments, static storage, union members, tuple elements
  and enum payloads cannot hold a resource value. Moving a local out, for example
  by `return a`, is planned.

### Raw memory

`p->init(...)` constructs a new value in the storage `p` points to, applying the
type's defaults and then the selected initializer, without destroying what was
there. `p->deinit()` destroys `*p` and leaves raw storage. They are the only way a
resource value lives in memory from `malloc`:

```c
File *p = malloc(sizeof(File))
p->init("a.txt")
...
p->deinit()
free(p)
```

As with `malloc` and `free`, nothing checks that storage is constructed once and
destroyed before it is freed. `init` and `deinit` are called this way only through
a pointer; a local or field is constructed and destroyed automatically.

`longjmp`, `exit` and signals do not run `deinit`, as they do not run `defer`.
