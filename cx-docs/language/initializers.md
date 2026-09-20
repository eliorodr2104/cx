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
Rules for earlier-field references in defaults and initializer delegation need a
precise definite-initialization design before implementation.

Every successful initializer must establish every required stored field. Before
that point, `self.field` may participate in initialization, but `self` must not escape
or be used as a fully initialized object. Cx must not fill non-nil class fields with
null simply to avoid a diagnostic.

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
