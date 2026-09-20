# Source Files, Type Declarations, and Implementations

Cx uses ordinary `.c` implementation files and `.h` headers. These extensions are
conventions for organization, not independent access-control domains.

## Primary declaration

A type's primary definition declares all stored fields and its visible API.

```c
// counter.h
#ifndef COUNTER_H
#define COUNTER_H

#module Counters

struct Counter {
    private(set) int value = 0

    void increment()
    ~mutating int current()
}

#endif
```

The primary member default is public. Stored private/internal fields also live here;
private access does not conceal their source text or physical layout.

## Reopening for implementation

```c
// counter.c
#module Counters
#include "counter.h"

struct Counter {
    void increment() {
        self.value += 1
    }

    ~mutating int current() {
        return self.value
    }

    void resetLocalState() {
        self.value = 0
    }
}
```

A block naming an already complete owned type is a continuation, not a second layout.
It may implement declared members and introduce private helper methods. No stored
fields may be added, for either structs or classes.

A newly defined type entirely inside one `.c` can still declare fields and method
bodies there. "Fields belong in the header" is the normal shared-type workflow,
not a prohibition on a standalone local type.

## Access and declarations

A member implementation inherits its declaration's access. A new implementation-only
method is private by default. An explicitly public/internal implementation requires
an appropriate visible interface declaration; otherwise report an error and suggest
adding one. Do not edit source headers during compilation.

An interface can be in the same `.c` when that is the chosen organization. Public
linkage never makes an unseen declaration automatically visible in another translation
unit. Headers or explicit compiler-interface mechanisms still supply declarations.

## Matching

Match owner type, base name, external labels, parameter types, generic requirements,
result type, error effect, receiver mutation, and accessor contract. Local parameter
names may differ. Default arguments belong in declarations, not repeated definitions.
An implementation cannot silently widen/narrow access or implement a different
throwing contract.

Multiple continuation blocks may implement distinct members, including across files
of one owner module. Exactly one non-inline implementation is allowed for a member.
Duplicate definitions and inconsistent primary layouts are build errors, not
link-order choices. Near-miss signature diagnostics must not silently convert an
intended public implementation into an unrelated private helper.

## Completeness and extensions

`struct Node;`, class forward declarations, and protocol forward declarations are
intended where a reference can be used without a complete interface. A value needs
its layout at by-value use; generic specialization needs the appropriate definition
or artifact. Exact class/protocol incomplete-use rules are G04.

An extension adds behavior/conformance explicitly. It cannot change storage and is
not permission for a foreign module to impersonate the original implementation.
See [Extensions](extensions.md) and [Modules](modules.md).
