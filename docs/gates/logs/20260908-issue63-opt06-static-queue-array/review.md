# Issue #63 OPT-06 static Queue-operation arrays

`ac.array` previously generated sources, memories, and nested collections only.
A generator such as `lambda bank: egress[bank].apply(...)` failed before the
ordinary Queue-operation parser, and `apply`/`merge` receivers accepted only a
bare Queue name. DavinciOO's four-way fabrics therefore had to copy the same
operation four times.

The frontend now expands a positive static array of any already supported
Queue-producing call. It substitutes the exact index through the existing
static evaluator, sends each generated expression through the ordinary parser,
requires one fresh Queue per element, and records a homogeneous static
collection. `apply`, `merge`, `credit`, `reorder`, and dependency receivers use
the common static Queue-reference resolver, so a constant array/map subscript
resolves to its canonical Queue identity.
There is no runtime Queue array, pointer dispatch, new ACIR operation, or backend
semantic path.

Positive coverage generates indexed operations with static-parameter extents,
different constants and latencies, and merges their static results. It checks
deterministic names and IR. Dynamic and out-of-range indices, shadowed generator
indices, non-Queue generators, element-name collisions, and non-Queue method
receivers fail closed. The full frontend suite ensures the generalized receiver
path does not capture unrelated high-level block methods.

The three dependent DavinciOO XBAR sources reproduce the original gap and now
lower using one four-entry egress template each. They remain unstaged for the
separate design PR so the generic framework fix lands first.

Independent review found no unresolved P0, P1, or P2 issues and recommended
approval after the framework/design verification boundary was made explicit.
