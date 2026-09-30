# M3 generic multi-assignment reconstruction follow-up

Status: backlog; reproduced on existing pre-source-parts helpers during the
M4 TU test design. This is not a new backend regression and is not fixed by the
source-parts package. Preserve the source semantics; do not treat splitting a
rule as an equivalent repair without proving its current/next behavior.

A real Python child with one rule containing `count = incoming` followed by
`outgoing = count` compiles as a source unit, but link refuses final
reconstruction. Native stderr is `final hardware ProposalGraph reconstruction
failed`, then `final hardware program view reconstruction failed`, then
`linked design is not reconstructible by the emit path; this source shape is
not supported yet`. No final output is published.

Minimal source files and observed stderr/exit code are archived under
`docs/gates/logs/20260930-m4-cpp-source-parts/multi-assignment-repro/`.
To reproduce, use tests/system/test_cpp_source_parts.py's source-unit `_compile_source`
helper separately on types.py, counter.py (types header), and test_counters.py
(types/counter headers), then `_link` the units with top
`demo.test_counters.TestCounters`. Helpers must come from the current checkout.

Next investigation: trace proposal provenance/use inventory through saved
final reconstruction for both assignments; add a source-level failing-first
regression, repair shared MLIR/link validation within the accepted C1/C2
contract, then prove both C++/RTL behavior. Do not loosen evidence checks, fix
only C++, add a Python semantic fallback, or reopen M2's bounded acceptance.
