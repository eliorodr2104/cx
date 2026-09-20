# Access Control

Cx access applies equally to struct and class members.

| Access | Intended audience |
| --- | --- |
| `public` | Any consumer with a visible declaration |
| `internal` | Code in the owning Cx module |
| `private` | The type implementation and owner-module extensions of that type |

Private means compiler-enforced access, not memory protection or secrecy of header
contents. Arbitrary raw C memory operations are still available.

## Defaults

Primary type members are public by default. Implementations retain the access of an
existing declaration. New helper methods introduced only in a continuation default
to private. Explicit public/internal bodies need matching interface declarations.

Ordinary top-level C declarations preserve C rules; a function in a `.c` is not
silently private. Top-level `static` retains C translation-unit linkage. `internal`
is the added module facility, not a new spelling for `static`.

## Read and write access

```c
class User {
    String name
    private String token
    private(set) int id
    internal(set) int generation
    internal private(set) int cacheVersion
}
```

Default read/write access is public. `private` restricts both. `private(set)` restricts
writing while retaining the declaration's read access. `internal private(set)` gives
internal read/private write. Write visibility cannot be broader than read visibility.

`let`/const immutability is separate from visibility: public read access does not
make an immutable binding writable.

## Mutation means more than `=`

Setter restrictions must cover compound assignment, increment, mutating method calls
on a stored value, mutable address-taking, and nested writeback through properties or
subscripts. A checker that rejects `obj.field = x` but allows an unrestricted mutable
pointer to the same field does not enforce the intended API.

Shallow read access to a stored raw pointer or class reference does not imply deep
immutability of its pointee. Explicitly define address-exposure rules in G08.

## Module identity

Access checks use declaration ownership, not the module of whoever included the
header. Owner-module extensions can access private state; unrelated module extensions
cannot. Modules are organizational/compilation contracts, not an adversarial security
boundary: a trusted build establishes module identities.

Outside a named module, the proposed conservative rule is local implementation access
within the translation unit. Treating every unnamed file as one giant privileged
module is not acceptable. This rule and top-level type defaults remain G04.

## Binary visibility

Access control, declaration visibility, linkage, and exported-symbol visibility are
different. A private method may need a cross-object-file symbol so another permitted
owner-module implementation can call it. See [Linkage](../abi/linkage-and-mangling.md).
