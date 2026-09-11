# Issue 105 inferred nested-rule capture evidence

Decision 0226 now derives nested-rule owner captures from static references to
direct typed module state. Capture order remains module declaration order, and
the frontend still rewrites every capture to an explicit hidden rule parameter
and call operand before ordinary owner, footprint, conflict, and lowering
analysis.

The explicit `nonlocal` spelling remains accepted only when it exactly matches
the inferred set. Focused negatives retain fail-closed behavior for stale or
partial explicit sets, untyped and late state, module inputs, parameter
shadowing, nested declarations, inter-rule calls, aliases, and generated-name
collisions. Explicit and inferred positive forms produce byte-identical raw
ACIR. The inferred form also reaches frozen QueueGraph and compiles as generated
gfsim C++20.

See `commands.txt`, the adjacent bounded logs, and `summary.json` for exact
evidence.
