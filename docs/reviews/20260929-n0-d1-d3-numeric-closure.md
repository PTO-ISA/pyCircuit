# Numeric N0-D1–D3 closure review

Date: 2026-09-29. Candidate checkout:
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`, HEAD
`82f161ea95336fdd15c238e103f13b9f823e8f28` plus the preserved migration
overlay.

## Scope

This review closes three private MLIR numeric packets after N0-C1:

- D1: exact `SourceRead - constant` and all six integer comparisons;
- D2: exact constant `and_bits` and explicit full-mask `low_bits` add/sub;
- D3: one checked `to_bits` integer boundary with a range check.

The packets remain below ProposalGraph, FinalProgram, C++/Verilog arithmetic,
Python numeric production, and public CLI admission. They reuse the whole-unit
clone, verify, and publish transaction established by N0-C1.

Final implementation/test bindings include:

- D1 verifier/lowering:
  `2240bb170689ed6b9e18f3e3bf9e10a239d3632a755468eeb781fe95758fcd09` /
  `031c31e4fae779b091e001cc35d527bf4cde3ba40c5a8972468cd72f180173a8`
- D1 proof/transaction tests:
  `cd84396ee9dcc5c9a0d8ecce3e60b398fbe0a0b317ee18275122358b1b1bccf4` /
  `7788529eb9992ae1650ef7eacdb6ea00e340d09e69f9c09056bc63c823e2793c`
- D2 verifier/lowering/test:
  `86ec09e0a5d4a4d019e93c6e950e119d5ebc951e61abc62efc1f7ab86b022eca` /
  `f6b1f9da2c643fbb7c61ccc78d7258817b8b37b65bf107d58f8979b20e11fa24` /
  `a66fc36d29b19655ff331c35bf8c211fd4ea5e493a5ce58f2d40928d6ff1794c`
- D3 verifier/lowering:
  `93935d26d0de5070cc894c9d922b0a4935db734d96a10eb38b298d135c28af80` /
  `a21608f927917a01ff35a6207cc2ba1d1e1640ccccdc0495de091750dbd434a6`
- D3 proof/transaction tests:
  `978c58c6234ddcfb92ab697455b0b7827231cde0c47388224d165c6f944cf28b` /
  `c7720ab918f6903a33565736193c4486eb67115d748239762f0cd235f7eb701d`

## Accepted behavior

D1 emits exact arbitrary-precision subtraction intervals and sign-aware
`arith.subi`, or a canonical bool result from `eq/ne/lt/le/gt/ge`. Unsigned
operands use unsigned comparison; any signed operand causes a sufficient signed
common representation. I/F/C/S identities, source ownership, exact SSA, bool
domain, rollback, and idempotence are independently verified.

D2a preserves infinite two's-complement `x & M`, including `-1 & 255`. D2b
accepts only an explicit mask `2^w-1` for `1 <= w <= 64`, keeps the eliminated
wide add/sub node in obligations, and does not invent a finite binding for that
intermediate. Arbitrary-precision constants are reduced modulo `2^w`. Shared
source/proof inventories, provenance, width-one domains, transactionality, and
multi-rule rollback are closed.

D3 keeps safety separate from demand:

```text
D = P && input_valid
S = A <= value && value < B
expect(condition=S, path=D, kind=range)
result_valid = D && S
```

The conversion occurs inside an exact `scf.if(result_valid)`. Its then branch
contains only the optional conversion and yield; its else branch contains only
the typed zero and yield. The proof, CheckBinding, CheckID, RequiredCheck,
SourceExpect, boundary target, source owner, and I/F/S identities are
cross-checked. Hoisting, duplicate guards, extra branch operations, sequential
checks, stale identities, and inactive unsafe reporting reject.

Each packet required independent implementation and test instances plus final
Sol/high code review. D1, D2, and D3 all received final **APPROVE / PASS**
verdicts after repairing provenance, width-one singleton, and guarded-region
findings.

## Harness correction and evidence

The backend closure harness now compiles and runs the generated minimal C++ and
Verilog artifacts before publication. It no longer emits synthesized V41–V44
JSON summaries. Unsupported execution fixtures fail without replacing the
requested result file. Candidate hashes:

- harness:
  `854abd8182db6f676fedc4008ed4153342ec9aba10d4a4204a417b6c51653f33`
- strict system tests:
  `f3362b4c68f1589e42c51085b05d9b57de7e84f2a0a7623cdd80e2db8ff0905e`

Fresh PM evidence:

- `ACIRSourceMathContractsTests`: 75/75 passed;
- complete configured ACIR lane: 19/19 CTest targets passed;
- clang-format and `git diff --check`: passed;
- all new handwritten native files remain below 600 lines;
- V41–V44 strict system lane: 1 passed, 7 failed fail-closed.

The seven red cases are real missing execution work: two numeric fixture
systems on both backends, two-system termination/completion, zero-rule
FinalProgram admission, complete events, schedule permutations, and source
reorder mapping. Static expected JSON is no longer accepted as evidence.

## Verdict

N0-D1, D2, and D3 are **accepted** as private compiler numeric closure.
W10 remains **request changes** because Python does not yet produce these
packets and FinalProgram/C++/Verilog do not yet execute their arithmetic. V41–
V44 remain red, so W11 remains blocked.
