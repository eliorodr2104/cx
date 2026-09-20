# Weak References

Weak storage observes an object without owning a strong reference. A weak load either
promotes a live object to a temporary strong reference or produces absence. It does
not return an unprotected raw pointer and hope the object survives the next instruction.

## Race-correct sequence

When the last strong release begins destruction, atomically make the object unavailable
for new weak promotion before user deinit/field destruction expose torn-down state.
Existing strong references obtained by earlier successful promotions keep the object
alive until they release it.

A side entry can outlive object storage while weak references remain. Clearing every
weak slot physically and marking one shared side entry dead are different possible
implementations; source semantics need not choose between them prematurely.

## Storage policy

Lazy/out-of-line weak bookkeeping is the preferred baseline so objects without weak
references do not allocate a full separate weak block. The runtime still needs a
safe lookup/discovery mechanism and synchronized initialization of that bookkeeping.
It cannot race first-weak creation against final strong release.

## Source surface

`weak User parent` is the current working spelling and yields optional load behavior.
Interaction with `User?`, read/write access, assignment, captures, manual-RC classes,
and restrictions on uninitialized weak fields requires precise qualification rules.
No `unowned` feature is assumed accepted merely because Swift offers one.

## Tests and measurement

Test simultaneous weak load/store/destruction, first-weak creation, no resurrection,
weak copies, partial construction, and cross-thread ownership under the supported
thread model. ThreadSanitizer can add evidence where supported; it is not a proof.
Only after these contracts pass does B03 compare representations and atomics.
