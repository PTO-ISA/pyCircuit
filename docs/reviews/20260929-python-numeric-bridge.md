# Python numeric producer bridge

Date: 2026-09-29. Status: accepted bounded source/lowering slice.

Candidate: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`,
HEAD `82f161ea95336fdd15c238e103f13b9f823e8f28` with the preserved uncommitted
migration overlay. Exact file hashes are recorded in
[candidate.json](../gates/logs/20260929-python-numeric-bridge/candidate.json).

## Delivered behavior

The native importer now accepts one unused local assignment in a nested rule:
`temporary = current + 7`, `temporary = current - 7`, or one of the six
comparisons with an integer literal. The rule has exactly one integer current
input and no next-state targets, checks, or observations.

`PythonImportNumeric.cpp` produces the existing verified C1/D1 source recipe:
SourceRead, MathFromBits, MathConstant, and MathBinary/MathCompare. The input
identity uses the current-name occurrence at slot zero; the lift uses that
occurrence at slot one. Constant and result identities retain their own source
occurrences. The importer emits the established proof scope and required nodes.
Python capture continues to transport syntax only.

The private source-unit harness supports `--lower-numeric`, which runs the
existing transactional lowering after source compilation and verification,
before writing outputs. Real Python tests check finite add/sub/cmpi operations,
four bindings, exact proof, bool domain, and identity preservation. An integer
larger than 2^53 is checked through both source emission and finite lowering.

Consumed temporaries, numeric next writes, assertions/observations involving
the numeric result, multiple expressions, chained comparisons, multiple runtime
operands, bool operands, shadowed input, and unsupported operators reject.
Numeric expressions inside unrelated log calls stay on the existing observation
diagnostic path. This last boundary was repaired after an existing regression
identified an overly broad numeric-route selector.

## Independent verification

Implementation and tests used separate Sol/medium instances. Independent
Sol/high code review returned **APPROVE — PASS**, with reviewed-scope hash
`0453c091ee95c34bbf2a947ff81c6fa3ceac8ed4ae55479edc8705bb8ebb8c5c`.
PM owns CMake and the private harness integration.

- New Python numeric system tests: 32/32 passed.
- Numeric, lexical, and observation system tests: 68/68 passed.
- Rebuilt configured ACIR lane: 19/19 CTest targets passed.
- Formatting, Python lint and `git diff --check`: passed.
- Initial source/harness tests failed before implementation; unsupported-shape
  cases already rejected. No required case was skipped.

Commands, checkout and exit status are preserved in
[system.log](../gates/logs/20260929-python-numeric-bridge/system.log) and
[ctest.log](../gates/logs/20260929-python-numeric-bridge/ctest.log).

## Remaining boundary and next packet

This accepts Python production of the existing unused scalar profiles. It does
not yet permit consuming numeric results as next values, conditions, checks,
or observations. Numeric Graph/FinalProgram/backend admission remains closed;
V41–V44 and W11 are not advanced by this slice.

The next packet is N0-U1: an owned `range(256)` register with
`state = (state + 1) & 255`. Before backend admission, retain the mask witness,
explicit integer boundary, original assignment UseID/RequiredUse, and exact
data/enable-to-target yield binding. Independent tests must enumerate all 256
inputs and reject redirected values, enables, targets, missing obligations and
dropped checks. After Graph/Final/backend support, the runtime oracle starts at
254 and observes 255, 0, 1 only after Xfer; Work must retain old Q.
