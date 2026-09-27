# Capability inventory independent review

Reviewer: governance_review, code-reviewer, gpt-5.6-sol, high. Verdict: revise.
Reviewed SHA-256: 850ff8206d1a6ee348f12263fddfe3e21789b07d47484ce759082d0e2712296a.
Frozen file: revision-a-matrix.txt. Date: 2026-09-27.

Five groups require correction:

1. Approval status is stale; split contract approval from implementation evidence. C1/C2/C3 foundations are now approved, only private capture has implementation evidence.
2. Split approved bool/record, fixed list, ordinary static parameters, ordinary state and portless root from unapproved Enum/value aggregates, dependent types, resources, external DUT/system/multiclock.
3. Add DFX/probes/trace (0140/0145/0121), incremental/scale/performance (0141/0147 and M6), and instance-aware combinational/timing/depth legality.
4. Expand retirement ownership/evidence to source-unit/header/link carriers, CMake/registries/bindings/aliases/modes/forwarders, public namespaces/install assets, runtime/consumer adapters, and SDK/schema/ABI/generated/package exports.
5. Correct mappings: memory 0114/0122; clocks 0126/reset domains; four-state 0121/uninitialized state; runtime/release 0149/0267/0268. Treat 0216 QueueProgram as retired carrier rather than target record/enum behavior; make 0275–0278 supersession explicit.

This is an inventory review, not validation of any new compiler/backend behavior.
