# Implementation Benchmark Checkpoints

Benchmark only implemented features with a defined correctness contract. Design
documents do not contain performance claims unless they link to reproducible results.

## Rules

First define correctness, lifetime, synchronization, and ABI compatibility. Compare
only implementations satisfying that contract. Record target/CPU, OS, compiler SHA,
flags, input sizes, allocation counts, code size, and reproducible test sources.
Report variance and realistic workload mixes, not only the most favorable microcase.

| Checkpoint | Milestone | Candidates and observations |
| --- | --- | --- |
| B01 Error channel | M13 | Aggregate vs hidden output vs target split; small/large success and errors; rare/frequent error; inlined and separate calls |
| B02 Callable elimination | M12 | Direct known call, stored callable, callback selected at runtime, captures/escaping; inspect allocations, indirection and code size |
| B03 ARC/weak | M11-M12 | Correct atomic/side-table layouts under the chosen thread contract; contention, no-weak objects, teardown and promotion |
| B04 Existential buffer | M16 | Inline capacities/alignment; class/small/large values; copying, mutation, code size and cache pressure |
| B05 Generic specialization | M9/M15/M16 | Monomorphized vs shared entries, witness calls, code size, build/cache time, duplicated specialization elimination |
| B06 COW/value views | M14/M18 | Copy/share/mutate workloads; scoped buffer access, retained substrings, allocation and copying counts |
| B07 Enum layout | M6/M16 | Explicit discriminant vs valid niches; over-aligned payloads; copy/destroy/switch; C export representation |
| B08 Real C port | M19 | Selected C dictionary unchanged under C/Cx, then opt-in Cx layers; functional tests before throughput, latency, and code-size comparisons |

## Performance invariants vs heuristics

No per-instance method storage and single evaluation of compound-assignment targets
are contracts, not optional benchmark outcomes. Inlining and whole-callable elimination
are opportunities. The optimizer can choose not to inline when code growth harms the
program, and a dynamically unknown target can require indirect dispatch.

"Two callable words" is a baseline materialized representation, not a bound on
captured data/allocation size. "Small existential" needs size *and alignment* eligibility.
Refcount atomicity is not optional merely because an unsafe alternative benchmarks
faster.

## Evidence artifacts

Store machine-readable results, compiler commands, normalized IR/assembly samples,
and a brief conclusion with the implementation change. Do not freeze an ABI solely
because one tiny example produces fewer instructions.
