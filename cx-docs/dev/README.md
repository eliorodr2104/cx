# Local Development Helpers

Fish shell helpers for working on the Cx fork. Neither is part of the compiler.

| Script | Purpose |
| --- | --- |
| [clangx.fish](clangx.fish) | A `clangx` function that runs the compiler built from this checkout |
| [hx.fish](hx.fish) | Launches Helix with this checkout's `clangd` on `PATH` |

## `clangx` everywhere

Build the compiler, then install the function once:

```fish
ninja -C build clang
ln -s (pwd)/cx-docs/dev/clangx.fish ~/.config/fish/functions/clangx.fish
```

`clangx` then works from any directory and resolves the checkout through the
symlink, so nothing is hard-coded. On macOS it also passes the active SDK as
`-isysroot`, because a locally built Clang has no default sysroot and linking
otherwise fails with `library 'System' not found`; an explicit `-isysroot` in
your own command line wins.

```fish
clangx -std=c11 demo.c -o demo
clangx -std=gnu23 -fsyntax-only src/*.c
```

The regression tree under `clang/test/Cx` is run by `lit`, which uses
`build/bin` directly and does not need this function:

```fish
./build/bin/llvm-lit -sv build/tools/clang/test/Cx
```
