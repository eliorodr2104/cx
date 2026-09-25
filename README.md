# Cx

Cx is an experimental Clang language mode that adds modern language features to C
without replacing C's source model, toolchain, or ABI. Existing `.c` and `.h` files
remain valid inputs, and plain C behavior stays available through the normal Clang
driver.

```c
#module Demo
#include <stdio.h>

(int, int) divmod(int a, int b) { return (a / b, a % b) }

struct Counter {
  int value
  void add(int by amount) { self.value += amount }
  ~mutating int current() { return value }
}

int main(void) {
  var c = Counter(value: 40)
  c.add(by: 2)

  let (q, r) = divmod(17, 5)
  printf("answer = %d, 17 / 5 = %d r %d\n", c.current(), q, r)
  return 0
}
```

## Why Cx

C++, Zig, C3 and similar languages ask you to move to a new language, and usually
rewrite existing code or its build. Cx takes the opposite approach. A C project adopts
it one file at a time: the rest of the code, the headers, the preprocessor and the
build stay as they are. Files without a `#module`, and everything declared in ordinary
C headers, keep their C symbols, so Cx and C code link together. New syntax is only
accepted where C could not read the code, so no valid C program changes meaning.

See the [language philosophy](cx-docs/language/philosophy.md) and the
[C continuity decision](cx-docs/design/0001-c-continuity.md).

## Current Status

Cx is experimental. The grammar, the symbol mangling and the ABI of Cx declarations
will change, and nothing here is ready for production code.

Implemented and tested so far:

- the `clangx` driver mode and explicit `clang -x cx` input selection
- `var`, `let`, and contextual `null`
- source ownership through `#module`
- Cx linkage, argument labels, overloads, and compound references
- struct methods, `self`, `~mutating`, continuations, and access control
- generated construction, field defaults, and custom initializers
- tuple types, literals, and destructuring
- implicit tag names and optional semicolons
- PCH, preprocessing, code completion, and tooling support

Each feature has an implementation record with tests, known limitations, and a
runnable example. The [milestone index](cx-docs/compiler/milestones/README.md) is the
authoritative status. Design documents also describe later work, so they must not be
read as a release claim.

## Install

Prebuilt compilers are attached to each
[release](https://github.com/eliorodr2104/cx/releases). The macOS build runs on Apple
silicon with macOS 14 or later and needs the Xcode Command Line Tools
(`xcode-select --install`) for the SDK and the linker. Save the example at the top as
`demo.c`, then:

```sh
curl -L https://github.com/eliorodr2104/cx/releases/download/cx-v0.1/clangx-v0.1-macos-arm64.tar.xz | tar -xJ
export SDKROOT="$(xcrun --show-sdk-path)"
./clangx/bin/clangx demo.c -o demo
```

The binaries are not signed. A download through `curl` runs as is; an archive
downloaded with a browser must first be cleared with
`xattr -dr com.apple.quarantine clangx`.

## Build

This repository is a fork of the LLVM monorepo. A partial clone avoids downloading
the full LLVM history up front:

```sh
git clone --filter=blob:none --single-branch -b cx/main https://github.com/eliorodr2104/cx
cd cx
```

Configure and build the compiler:

```sh
cmake -S llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_ENABLE_PROJECTS=clang

ninja -C build clang
```

Add `clang-tools-extra` to the projects and build `clangd` for editor support. A
faster macOS configuration with shared libraries is described in
[Clang Integration](cx-docs/compiler/clang-integration.md).

## Try Cx

Save the example above as `demo.c`, then compile and run it:

```sh
./build/bin/clangx -isysroot "$(xcrun --show-sdk-path)" demo.c -o demo
./demo
```

On macOS a locally built Clang has no default SDK, so `-isysroot` is required to find
the system headers and libraries. Drop it on Linux. `clangx` reads `.c` files as Cx;
`clang -x cx` does the same through the ordinary driver.

The optional Fish helpers in [`cx-docs/dev`](cx-docs/dev/README.md) provide a
checkout-aware `clangx` command and Helix integration.

## Test

Run the Cx regression suite after compiler changes:

```sh
./build/bin/llvm-lit -sv build/tools/clang/test/Cx
```

Run broader Clang tests when a change affects shared C parsing, semantic analysis,
serialization, code generation, or tooling. The
[testing guide](cx-docs/compiler/testing.md) explains the expected coverage for each
kind of feature.

## Documentation

- [Cx documentation index](cx-docs/README.md)
- [Language guide](cx-docs/language/README.md)
- [Compiler design](cx-docs/compiler/README.md)
- [Implementation roadmap](cx-docs/compiler/implementation-roadmap.md)
- [Decisions](cx-docs/DECISIONS.md)
- [Open design gates](cx-docs/OPEN-ISSUES.md)

## License

Cx is built on LLVM and Clang and is distributed under the same license, the Apache
License v2.0 with LLVM Exceptions. See [LICENSE.TXT](LICENSE.TXT).

For upstream LLVM build instructions, contribution guidance, and project policies,
see the [LLVM documentation](https://llvm.org/docs/) and the
[LLVM project repository](https://github.com/llvm/llvm-project).
