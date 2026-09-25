# Cx

Cx is an experimental Clang language mode that adds modern language features to C
without replacing C's source model, toolchain, or ABI. Existing `.c` and `.h` files
remain valid inputs, and plain C behavior stays available through the normal Clang
driver.

This repository is a fork of the LLVM monorepo. Cx compiler work lives in `clang`,
Cx regression tests live in `clang/test/Cx`, and the language and implementation
documentation lives in [`cx-docs`](cx-docs/README.md).

## Current Status

The merged compiler work currently includes:

- the `clangx` driver mode and explicit `clang -x cx` input selection
- `var`, `let`, and contextual `null`
- source ownership through `#module`
- Cx linkage, argument labels, overloads, and compound references
- struct methods, `self`, `~mutating`, continuations, and access control
- generated construction, field defaults, and custom initializers
- PCH, preprocessing, code completion, and tooling identity support
- implicit tag names and optional semicolons

Each completed feature has a scoped implementation record with tests, known
limitations, and a runnable example. See the
[milestone index](cx-docs/Compiler/milestones/README.md) for the authoritative
status. Design documents describe both implemented behavior and later work, so they
must not be read as a release claim.

## Build

Configure a development build with assertions enabled:

```sh
cmake -S llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra'

ninja -C build clang clangd
```

The project also documents a faster macOS configuration with shared LLVM and Clang
libraries in [Clang Integration](cx-docs/Compiler/clang-integration.md).

## Try Cx

Save this as `demo.c`:

```c
#module Demo

int main(void) {
  let answer = 42
  return answer == 42 ? 0 : 1
}
```

Compile it with the explicit language mode:

```sh
./build/bin/clang -x cx demo.c -o demo
./demo
```

The optional Fish helpers in [`cx-docs/dev`](cx-docs/dev/README.md) provide a
checkout-aware `clangx` command and Helix integration.

## Test

Run the Cx regression suite after compiler changes:

```sh
./build/bin/llvm-lit -sv build/tools/clang/test/Cx
```

Run broader Clang tests when a change affects shared C parsing, semantic analysis,
serialization, code generation, or tooling. The
[testing guide](cx-docs/Compiler/testing.md) explains the expected coverage for each
kind of feature.

## Documentation

- [Cx documentation index](cx-docs/README.md)
- [Language guide](cx-docs/language/README.md)
- [Compiler design](cx-docs/Compiler/README.md)
- [Implementation roadmap](cx-docs/Compiler/implementation-roadmap.md)
- [Decisions](cx-docs/DECISIONS.md)
- [Open design gates](cx-docs/OPEN-ISSUES.md)

## LLVM

For upstream LLVM build instructions, contribution guidance, community links, and
project policies, see the [LLVM documentation](https://llvm.org/docs/) and the
[LLVM project repository](https://github.com/llvm/llvm-project).
