# 0012 - Uniform ABIs and Measured Optimization

## Status

Baseline representations accepted; exact layouts are implementation checkpoints.

## Context

First-class closures, object ownership, and protocol erasure need reliable general representations, while simple source should still optimize to direct code.

## Decision

Use a one-reference class model with inline ownership/metadata header on ordinary targets; callable entry+context; uniform existential containers with optional inline storage; late lowering into ordinary LLVM IR. Reuse ownership/value mechanisms instead of inventing independent lifetimes for each abstraction.

## Consequences

One materialized static type has one ABI. Capture/box allocation size is not bounded by the size of its reference pair. Known calls may disappear through inlining, but runtime-selected callbacks remain valid. Weak promotion must be race-correct before layout speed is compared.

## Implementation gates

B01-B08 measure implemented alternatives. G06-G09 establish correctness and module/ownership contracts first. No benchmark is claimed in this documentation and no LLVM core change is presumed necessary.

## Related contracts

[ABI index](../abi/README.md), [Runtime index](../runtime/README.md), [Benchmarks](../Compiler/benchmarking.md).
