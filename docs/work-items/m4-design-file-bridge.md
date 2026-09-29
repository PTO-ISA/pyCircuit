# M4: source-unit and serialized-design bridge

Status: paused for the user-requested design/testbench boundary review. Base: `6b514f90`, branch `codex/gfsim-source-units`.
The user requested continued implementation after bounded M2 acceptance. This
packet implements existing C2/C3 native operations, not a new source language,
public driver contract, or M5 cutover.

## Bounded outcome

Read explicit separately compiled source body/interface pairs from disk, link
and materialize the existing supported root to a verified final design file,
then reparse that file in another process and use the same final verifier and
C++/Verilog emitters. No fixture names, hidden source bodies, QueueGraph fallback
or whole-system compilation followed by splitting.

The private development tool is `acir-design-harness`, with two exclusive modes:

- Link: repeated `--body` and `--header` in corresponding order, `--top` matching
  the qualified root already admitted by the existing linker, `--target final`,
  and `--output` naming a new scratch file.
- Emit: `--design`, `--target cpp|verilog`, and `--output` naming a new scratch file.

Existing/symlink output is rejected. All native validation occurs before file
creation. These are scratch intermediate files; managed C3 publication remains
owned by the existing Python publication engine, not this helper. No crash-safe
managed replacement or generated source-group package is claimed here.

## Ownership and verification

- `m4_program_bridge`: gpt-6-luna/high, new native helper directory only.
- `m4_program_tests`: independent gpt-6-luna/high, new system test only.
- PM: parent CMake integration, checkout-local builds, documentation, acceptance.
- Independent code review: reused native reviewer, separate from implementation.

Tests use real Python per-source capture and native source compilation, then
separate link/emit processes. Cover both backends, serialized-final rejection,
explicit root mismatch, malformed unit sets, and existing output preservation.
Relevant prior M2 core regressions remain required if shared compiler code changes.

## Non-goals

Public `pycircuit` command replacement, managed unit receipts/depfiles, generated
source-owned C++ groups/CMake, runner ABI packaging, installation, parallel
simulation and complete SDK are later slices. Do not mark M4 complete from this
bridge alone. Existing public routes are not fallback for this private path.

## User correction — design and testbench boundary

The latest user instruction names the hardware artifact `design_top.ac` and
requires design, testbench and framework/runtime to remain separate. This
renaming adds no IR primitive and does not establish that separation by itself.
The present positive fixture links a source test system containing stimulus and
checks; it is evidence for closed-system simulation, not an independently
deliverable synthesizable DUT. Public driver wiring and further IR extensions
are paused until the architecture audit identifies the exact boundary. Existing
`FinalProgram` is an internal C++ container name, not a new source-language
program model; any structural replacement needs its own reviewed contract.

The canonical hardware-design artifact name is `design_top.ac`. The current
V41 closed-system fixture instead writes `closed_system_testbench.ac`: its
stimulus/phase/check rules are part of that test hierarchy. Renaming that
fixture output to design_top would not make it an independent DUT. No new
`ac.design`/`ac.testbench` op or role attribute is authorized by this naming fix.
