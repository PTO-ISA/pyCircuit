---
doc_id: RV32-DOC-ACCEPTANCE
status: draft
authority: normative
owner: rv32i-demo
---

<!-- markdownlint-disable MD022 -->

# RV32I acceptance and traceability

Revision 1, candidate. The clauses are design requirements for review, not an
approval or a claim that the implementation has passed them. Git records the
exact text; clause IDs persist across revisions. Implementation baseline:
`c00f45c1b023c89279c7ed7a9e519c83f8f456ba`.

## Independent ISA and EEI expectation {#RV32-VER-ISA-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=RV32-REQ-ISA-001,RV32-REQ-EEI-001,RV32-SPEC-ISA-001,RV32-SPEC-EEI-001 -->

The verifier MUST compare each retirement PC/word, rd/write/raw result,
load/store address/mask/data, trap and final GPR/PC/RAM against the pinned Spike
or a separately authored selected-EEI expectation. FENCE.I rejection MUST remain
separate from Spike agreement. Program families MUST cover all 40 named base
instructions and legal/reserved encoding boundaries, sign/x0/subword behavior,
misalignment, illegal, ECALL/EBREAK and access faults. See
[program generation](../verification/prepare.py) and
[independent comparison](../verification/verify.py). Existing results in
[RESULTS](../RESULTS.md) predate these candidate clause IDs and are historical
implementation evidence, not approval or a new Spec-conformance result.

## Pipeline and dependency observation {#RV32-VER-PIPE-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=RV32-REQ-PIPE-001,RV32-SPEC-STATE-001,RV32-SPEC-HAZARD-001 -->

The cycle checker MUST separately assert overlapping five-stage occupancy,
one independent ALU retirement per cycle after fill, the declared load-use
bubble, forwarding/store dependencies, redirect cancellation and sustained
memory waits/recovery. The directed throughput and smoke programs and the
saved cycle records are the minimal witnesses; terminal-state equality is
insufficient. See [cycle checks](../verification/suite.py).

## Effects and reset {#RV32-VER-EFFECT-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=RV32-REQ-EFFECT-001,RV32-SPEC-BUS-001,RV32-SPEC-COMMIT-001 -->

The verifier MUST match external store count, aligned address, byte mask and
written bytes to independently retired stores. It MUST check wrong-path and
faulting instructions have no forbidden effects, request stability under waits,
and reset during a waiting store suppresses its completion. Full state and RAM
are checked after reset/recovery. A missing or failed trace is not PASS.
See [scenario runner](../verification/suite.py) and
[store/state checker](../verification/verify.py).

## Same-source execution views {#RV32-VER-VIEWS-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=RV32-REQ-VERIFY-001,RV32-SPEC-VIEWS-001 -->

The public installed CLI MUST compile/link/emit/run the same program image
under functional/timing and cpp/verilog. The verifier MUST compare independent
retirement expectations, complete state, native/RTL cycle records and final IR
identity, retaining failures and timeout status. At least the compiled C array
application MUST be replayed after binding this Spec candidate. A source-only
comment change may reuse unchanged generated-logic evidence only after actual
artifact comparison; it MUST NOT relabel the previous campaign as a new run.
See [suite](../verification/suite.py), [raw checker](../verification/check_results.py)
and [clean consumer](../verification/clean_consumer.py).
