# Single ROB generated-model tests and replay

Decision 0222 design-local validation and Decision 0228 replay evidence.
Fixed environment: pyc6; generated code links this checkout's gfsim build.
No ROB implementation or framework semantics changed.

The new test_rob_single.py selects rob_system, generating one ROB instance
with four input Queues, two output Queues, five operations and seven state
Tables. The shared C++ driver now selects single/dual layouts explicitly at
compile time. Flow count determines port and state offsets; expected-result
checks remain shared. The dual-flow conflicts scenario remains dual-only.

Four scenarios validate independent expected results, reset before recording,
full scan/activation state and commit timelines, and normal/recorded byte parity:

- capacity: fill all 16 slots, block allocation, reverse completion order,
  ordered handoff, wait for reliable ack, generation reuse and duplicate ack;
- backpressure: allocation and committed output blocking, then resume;
- invalid: stale/wrong identities and duplicate completions, non-durable,
  incomplete or mismatched confirmations;
- recovery: flush without pending handoff, preserve published-but-unconsumed
  handoff, block allocation while awaiting ack, reject late responses and resume.

single.log records the four-case result. dual-regression.log records all five
existing dual-flow cases passing after the shared driver/helper refactor.
The independent reader checks exact result/exception widths and verifies the
operation paths are exclusively rob_system/rob[0]/{rule}.

Browser validation of recovery/replay.html passed offline, including play,
pause, step, backward/forward seek, atomic state changes and exact nested u64
1152921504606846977 (inst.original_pc). Screenshot is in browser/nested-fields.png.
Chromium required the already-established sandbox escalation for system calls.

Artifacts: .pycircuit_out/davincioo-rob/20260908-single/index.html, shared generated
C++/MLIR/driver/compile command, and each scenario's execution.pyctrace,
activation.pyctrace, projection.bin and replay.html. The index describes all
four cases in Chinese. Reproduction:

```sh
/home/lc/.codex/skills/pyc6/scripts/run.sh python -m pytest designs/davincioo/tests/spe/ooo/test_rob_single.py -q --tb=short
/home/lc/.codex/skills/pyc6/scripts/run.sh env PYC_DAVINCIOO_ROB_OUT=/home/lc/pyCircuit/.pycircuit_out/davincioo-rob/20260908-single-dual-regression python -m pytest designs/davincioo/tests/spe/ooo/test_rob.py -q --tb=short
```

PYC_DAVINCIOO_SINGLE_ROB_OUT overrides the single-instance artifact directory.
Format checks pass; the sandbox-stalled multi-file Black process was interrupted
and replaced by individual Black checks for both Python files (format.log).
No commit or external publication was made.
