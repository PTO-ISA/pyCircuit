# Issue #63 OPT-04 scalar first-choice priority encoding

An effect-free `ac.find(..., where=...)` already produces a candidate mask, but
generated C++ scanned the Table again to select the first set bit. The generator
now recognizes only the proven scalar 1..64-entry, empty-key, no-snapshot case
and emits one shared low-first `gfsim::priorityEncode`; index and valid continue
to use the existing per-expression cache. Min/max, choose-key snapshots, and
65+ entry word-array masks retain the general selection scan.

The shared priority encoder now masks to its declared width and uses C++20
leading/trailing bit scans. Zero remains `index=0, valid=0`; low/high direction,
index width, and multi-hit results are unchanged. Width 1/16/64 generated
policies compile and execute; fallback source tests prove min, max and snapshot
retain one selection loop and width 65 retains its match plus choose loops.
The hierarchy-preserving generator includes the shared encoder header, and the
full I2 generated behavior matrix remains green.

The QueueGraphPlan verifier now requires inline choose masks to come from a
match on the same Table with exact entry width, exact index/valid types,
canonical policy/key metadata, balanced result pairs, and index-before-valid
ordering. The generator and verifier share one recursive contract key. Cache
reuse is limited to each index/valid pair so independent keyed choices retain
their own snapshot reservations. Generated-source regressions compile the full
firing policy and distinguish nested slice/mask keys that previously collided.

The shared Table selection cache preserves the existing mask interface order:
`CandidateSet` uses its one-word scan, scalar-only masks may use conversion, and
custom masks that expose `.test(index)` retain that behavior.

For the current I2 H3 source, generated C++ changes from 192,770 to 191,392
bytes, Table scan loops from 10 to 5, and shared priority-encoder calls from 0
to 5. Five-run local Apple-clang `-O3` diagnostics preserve identical checksums:
random masks improve from median 0.998821 to 0.916325 ns/iteration; sparse
one-hot masks improve from 10.0633 to 0.356067 ns/iteration. These timings are
measurements, not semantic pass thresholds.
