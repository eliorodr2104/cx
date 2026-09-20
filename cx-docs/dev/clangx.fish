# Cx compiler wrapper: runs the clangx built from this checkout.
#
# Install once, then `clangx` works from any directory:
#
#   ln -s (pwd)/cx-docs/dev/clangx.fish ~/.config/fish/functions/clangx.fish
#
# Build it first with: ninja -C build clang

function clangx --description "Cx compiler (clangx) from this llvm-project checkout"
    set -l root (path resolve (status filename)/../../..)
    set -l bin $root/build/bin/clangx

    if not test -x $bin
        echo "clangx has not been built yet" >&2
        echo "expected: $bin" >&2
        echo "build it with: ninja -C $root/build clang" >&2
        return 1
    end

    # A locally built clang has no default sysroot on macOS, so linking fails
    # with "library 'System' not found". Supply the active SDK unless the
    # caller already chose one.
    set -l sysroot
    if test (uname) = Darwin; and not string match -qr -- '^-{1,2}isysroot' $argv
        set -l sdk (xcrun --show-sdk-path 2>/dev/null)
        if test -n "$sdk"
            set sysroot -isysroot $sdk
        end
    end

    $bin $sysroot $argv
end
