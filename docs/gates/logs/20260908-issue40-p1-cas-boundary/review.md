# Issue #40 P1 CAS/Wire boundary

`CycleAwareDomain.create_signal()`, `create_const()`, and `create_reset()` now
return `CycleAwareSignal` at the domain's current occurrence. The corresponding
`CycleAwareCircuit` convenience factories follow the same contract. Raw Wire
values remain explicit at `CycleAwareCircuit.input()`/`const()` and `wire_of()`
boundaries.

The domain factory contract is covered in both direct construction and the
inspectable JIT path, including hardened `pyc.reset_active` and `pyc.select`.

Top-level `mux()` requires a cycle-aware anchor and always returns CAS. A pure
Wire call fails with a diagnostic directing the author to
`pycircuit.structural.mux()`, which always returns Wire. BypassUnit and the
structural IssueQueue helpers migrated to that namespace. The raw-Wire FM16 NPU
and switch selection sites use the same explicit helper. Direct eager and JIT
build probes pass for all four consumers.
