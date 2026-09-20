#!/usr/bin/env fish

set root (git -C (dirname (status filename)) rev-parse --show-toplevel 2>/dev/null)

if test -z "$root"
    echo "error: could not find repository root"
    exit 1
end

set clangd "$root/build/bin/clangd"

if not test -x "$clangd"
    echo "error: clangd has not been built yet"
    echo "expected: $clangd"
    echo
    echo "build it with:"
    echo "  ninja -C build clang clangd"
    exit 1
end

# Local to this script and exported to Helix + child processes.
set -lx PATH "$root/build/bin" $PATH

if test (count $argv) -eq 0
    exec hx "$root"
else
    exec hx $argv
end
