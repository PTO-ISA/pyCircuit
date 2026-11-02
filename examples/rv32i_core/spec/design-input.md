---
doc_id: RV32-DOC-INPUT
status: draft
authority: normative
owner: rv32i-demo
---

<!-- markdownlint-disable MD022 -->

# RV32I core: design input

Revision 1, candidate. The clauses are design requirements for review, not an
approval or a claim that the implementation has passed them. Git records the
exact text; clause IDs persist across revisions. Implementation baseline:
`c00f45c1b023c89279c7ed7a9e519c83f8f456ba`.

## Purpose {#RV32-INTENT-001}
<!-- ndf: kind=requirement modality=must refinement=L0 domain=architecture status=draft -->

The example MUST execute real RV32I programs through a single-issue, in-order
five-stage IF/ID/EX/MEM/WB core, with simultaneous in-flight instructions. Its
functional, timed-native and RTL views MUST derive from one hardware source.
An ISS is a verification oracle and MUST NOT supply DUT results.

## Architectural authority {#RV32-REQ-ISA-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=architecture status=draft refines=RV32-INTENT-001 -->

The architectural contract MUST implement RV32I version 2.1, fixed ISA manual
[v20260120](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html):
32 32-bit GPRs with immutable x0, PC, the 40 base instructions and their
fixed-width integer behavior. Optional extensions are not implied. Reserved
encodings use the explicit illegal-instruction EEI policy in the Spec.
The ISA is the authority for instruction results; matching another model is
insufficient if that model disagrees with the selected ISA.

## Execution environment {#RV32-REQ-EEI-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=platform status=draft refines=RV32-INTENT-001 -->

The platform MUST define reset, byte order, memory accessibility, request
completion, faults and termination. Every requested transaction MUST remain
stable until completion or reset cancellation. Misaligned accesses and
unsupported instruction encodings MUST cause precise traps, not silent NOPs.
The demo selects a little-endian, blocking Harvard environment and a precise
halt-on-trap policy; these are EEI choices rather than portable ISA mandates.

## Dependency and throughput {#RV32-REQ-PIPE-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=pipeline status=draft refines=RV32-INTENT-001 -->

Independent ALU instructions with ready memory MUST overlap in five stages
and sustain one retirement per cycle after fill. Data dependencies, load-use,
store-data dependencies, control redirects and memory backpressure MUST retain
program results without overwriting live instructions. Any stall or flush MUST
be justified by those conditions rather than by serializing every instruction.

## Precise observable effects {#RV32-REQ-EFFECT-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=architecture status=draft refines=RV32-INTENT-001 -->

Architectural register writes, stores and trap events MUST appear in program
order. A store MUST update external memory exactly once. Wrong-path, faulting,
reset-cancelled and retried instructions MUST NOT produce prohibited effects.
The oldest exception MUST prevent younger architectural effects; reset MUST
win over a same-edge memory completion.

## Cross-view acceptance {#RV32-REQ-VERIFY-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=verification status=draft refines=RV32-INTENT-001 -->

Acceptance MUST compare the same machine code and initial memory against an
independent ISA expectation, then compare retirement, full architectural state
and external memory across both policies and both backends. Cycle behavior
MUST have independent checks for throughput, hazards, waits and reset.
Functional wait compression MUST preserve logical events and virtual-cycle
boundaries. Compilation or synthesis success alone MUST NOT establish this
acceptance or approval of the design contract.
