# VerilogEval pyCircuit authoring findings

2026-10-19 update: see the [compiler audit](compiler-audit-20261019.md) for current
owners and fixes. Fixed runtime counts are now admitted; helper-loop shift
preflight is repaired. `vector100r` still fails constant-resource reservation
(shift form at iteration 44; concat form at 64). Managed sequential RTL export
remains unsupported by the upstream-TB adapter. Earlier diagnostics below are
retained as historical evidence, not current admission guidance.

This is assisted pyCircuit port coverage, not LLM pass@1. The source dataset is
VerilogEval 2023 Human, pinned by `benchmark/sources.lock.json`. Initial authoring
used public descriptions and module headers. The `bugs_mux2` correction is
oracle-informed: inspecting the public test field unintentionally exposed its
embedded reference module. No other canonical solutions or reference DUT bodies
were inspected in the Astra authoring batch. Preserve this exception in reports.

## Current source inventory

The manifest has 88 rows: 70 combinational candidates, 17 sequential candidates
and one unsupported loop reproducer. Single-value results are scalars; Structs
group several related outputs. Sequential sources declare Module-owned state
and register source-global Rules with explicit state arguments. Rule definition
location does not establish storage ownership. There are no authored clock/reset
pins, DFF/DFFE instances or proposal/commit wires in these sources.

Original RTL data-port names are mapped in `inputs`; physical clocks and resets
are described in `domain`. A missing original reset is recorded as null, not
implemented by injecting a reset pulse. The 17 sequential sources target one
rising-edge domain. Asynchronous reset, falling-edge, latch and dual-edge tasks
remain outside this batch.

## Reproduced boundaries and corrections

| Source | First observed boundary | Current treatment |
| --- | --- | --- |
| `bugs_mux2` | Public prompt's buggy expression implies `sel=1` selects `b`; the embedded public oracle selects `a`. Initial prompt-derived candidate failed original tests. | Corrected selector polarity to satisfy that oracle. This row is oracle-informed and must not be described as prompt-only generation. |
| `dff8` and all sequential candidates | Generated RTL exposes `pyc_phase`, `pyc_root_commit_ok` and check/error ports in addition to the compiler-generated clock/reset. | Compile/link/dual emit pass. Ordinary clock/reset name mapping does not implement the managed PREPARE/check/commit lifecycle. The benchmark adapter must reject this boundary until generic supported export exists; never tie private controls off. |
| `lfsr5`, `lfsr32` | Source compile rejects `(sequence >> 1) ^ (20 if feedback else 0)` (5-bit case) with `unsigned boundary requires authoritative bits, Boolean or Integer provenance`. | Explicitly type the selected `mask` as `ac.u5`/`ac.u32` before XOR. Both versions compile/link/dual emit. Original sources and exact diagnostics are retained in gate evidence. |
| `vector100r` | A pure helper loop using fixed shifts rejects with `bounded for binary operation is unsupported` and `bounded for capability preflight failed`. | Retain the readable loop as an unsupported source reproducer. A second form using static slices and `ac.concat` gets further but fails at iteration 64 with `Table query constant bytes budget exhausted`. No manual 100-lane expansion or compiler budget change was added. |
| `m2014_q4j` | Original comparison/concat draft rejected Boolean as explicitly unsigned fixed bits; the intermediate one-field `Carry` Struct was unnecessary. | A pure helper widens the two operands into ordinary `ac.u5` locals, then adds them. Scalar result; compile/link/dual emit pass. |
| `vector3` | Original one-field `Constant` Struct existed only to type a literal for concat. | A pure helper uses `one: ac.u1 = 1`; no wrapper type. Compile/link/dual emit pass. |
| `mux2to1v` initial draft | `ac.u100` rejected; convenience aliases end at `u64`. | Use `ac.bits[100]`. |
| `fsm3comb` initial draft | Statement branch assignments in a pure Module rejected with `behavioral module supports persistent declarations and module-scope rule calls`. | Existing conditional-expression implementation compiles. Pure helper statement branches are the current readable route to evaluate for future authoring. |
| `mux256to1`, `mux256to1v` | Dynamic packed selection is not in the current Bits contract; shifts require static counts. | Not authored. Record the generic dynamic selection gap instead of 256 fixture-specific alternatives. |

`gatesv100` needs no 100-lane unrolling: two static slices express adjacent pairs,
and one concat expresses the wraparound neighbor. It now compiles through both
targets. `vector4` still needs repeated sign operands because starred concat
arguments are unsupported; a generic bit replication operation could simplify it.

## Verification and evidence

The Astra batch records commands, exit codes, source hashes, source-unit outputs,
verified final ACIR and both backend artifacts under ignored
`docs/gates/logs/verilogeval-astra-authoring-20261018/`. Each source has a
`result.json`; failed draft reproducers are retained separately. The explicit
`PYCIRCUIT_SOURCE_COMPILER`, `PYCIRCUIT_LINKER` and `PYCIRCUIT_EMITTER` paths select
this worktree's `.pycircuit_out/toolchain/build/bin` tools.

Command shape: `python -m pycircuit.cli compile -c ABSOLUTE_SOURCE --source-root
ABSOLUTE_SOURCE_ROOT --package-prefix verilogeval -o UNIT`, then explicit
`link UNIT --top verilogeval.STEM.Top -o FINAL.ac`, then `emit FINAL.ac --target
cpp|verilog -o OUTPUT`. Both targets consume the same final artifact.

Compile/link/emit admission and original-testbench behavior are separate claims.
The root benchmark runner owns the final unchanged Icarus run and its coverage
report. Sequential source admission does not imply adapter or behavioral support.
A previous 65-source compile pass or 64/65 original-test result is historical,
not certification of the current reformatted and expanded source set.
