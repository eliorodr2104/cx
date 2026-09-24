# Launch Helix with this checkout's Cx-aware clangd, so `.c` and `.h` files are
# read as Cx instead of drowning in errors about `#module`, `var`, `let` and
# labels.
#
# The `-x cx` in .clangd matters most for headers: left to itself clangd reads
# a `.h` as an Objective-C++ header, where every Cx construct is an error.
#
# Install once, then `hxcx` works from any directory:
#
#   ln -s (pwd)/cx-docs/dev/hxcx.fish ~/.config/fish/functions/hxcx.fish
#
# Build clangd first with: ninja -C build clang clangd
#
# Use `hx` for ordinary C; this function is only for Cx sources.

function hxcx --description "Helix with the Cx language server from this checkout"
    set -l root (path resolve (status filename)/../../..)
    set -l clangd $root/build/bin/clangd

    if not test -x $clangd
        echo "clangd has not been built yet" >&2
        echo "expected: $clangd" >&2
        echo "build it with: ninja -C $root/build clang clangd" >&2
        return 1
    end

    # clangd reads .clangd from the file's directory and every parent, so one
    # file at the root of the tree being edited covers everything under it.
    # Never touch a .clangd that already exists: it is the user's.
    set -l target (test (count $argv) -gt 0; and echo $argv[1]; or pwd)
    if test -d $target
        set target (path resolve $target)
    else
        set target (path dirname (path resolve $target))
    end

    set -l config (__hxcx_find_config $target)
    if test -z "$config"
        printf 'CompileFlags:\n  Add: [-x, cx]\n' > $target/.clangd
        echo "hxcx: wrote $target/.clangd so clangd reads this tree as Cx"
    else if not grep -q -- '-x.*\bcx\b' $config
        # Never edit a .clangd that is the user's; say what is missing instead.
        # Without `-x cx`, clangd reads a .c file as C and a .h file as an
        # Objective-C++ header, and every Cx construct becomes an error.
        echo "hxcx: $config does not add '-x cx'; Cx files will not parse" >&2
        echo "      add this to it:" >&2
        echo "        CompileFlags:" >&2
        echo "          Add: [-x, cx]" >&2
    end

    # Exported so Helix and the language server it spawns both see it.
    set -lx PATH $root/build/bin $PATH

    # Not `exec`: that would replace the shell, and quitting Helix would then
    # close the terminal window along with it.
    if test (count $argv) -eq 0
        hx
    else
        hx $argv
    end
end

# The .clangd covering $argv[1], searching it and every directory above, or
# nothing when there is none.
function __hxcx_find_config
    set -l dir $argv[1]
    while test -n "$dir"; and test "$dir" != /
        if test -f $dir/.clangd
            echo $dir/.clangd
            return 0
        end
        set dir (path dirname $dir)
    end
    if test -f /.clangd
        echo /.clangd
    end
end
