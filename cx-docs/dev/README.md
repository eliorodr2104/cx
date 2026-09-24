# Local Development Helpers

Fish shell helpers for working on the Cx fork. Neither is part of the compiler.

| Script | Purpose |
| --- | --- |
| [clangx.fish](clangx.fish) | A `clangx` function that runs the compiler built from this checkout |
| [hx.fish](hx.fish) | Launches Helix with this checkout's `clangd` on `PATH` |
| [hxcx.fish](hxcx.fish) | The same, and configures `clangd` to read `.c` files as Cx |

## `clangx` everywhere

Build the compiler, then install the function once:

```fish
ninja -C build clang LTO
ln -s (pwd)/cx-docs/dev/clangx.fish ~/.config/fish/functions/clangx.fish
```

`LTO` is in that list because the driver passes `-lto_library` to the linker on
Darwin. Without it every link prints `ld: warning: ignoring -lto_library ...
file does not exist`. The warning is harmless -- the link succeeds -- but it is
noise on every build.

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

## Editing Cx sources

`hx` reads a `.c` file as plain C, so every `#module`, `var`, `let`, `null` and
argument label becomes an error in the editor. `hxcx` fixes that:

```fish
ninja -C build clang clangd LTO
ln -s (pwd)/cx-docs/dev/hxcx.fish ~/.config/fish/functions/hxcx.fish
```

```fish
hxcx src/demo.c
```

It puts this checkout's `clangd` first on `PATH` and, if no `.clangd` is
already in scope, writes one next to what you are opening:

```yaml
CompileFlags:
  Add: [-x, cx]
```

`clangd` reads `.clangd` from a file's own directory and every directory above
it, so one file at the root of a tree covers everything under it. An existing
`.clangd`, anywhere up that chain, is never touched — it is yours. Keep using
`hx` for ordinary C.

Rebuild `clangd` after changing the frontend, or the editor will keep
diagnosing against the old language rules:

```fish
ninja -C build clangd
```
