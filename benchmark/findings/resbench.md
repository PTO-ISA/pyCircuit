# ResBench port findings

2026-10-19 update: the [compiler audit](compiler-audit-20261019.md) supersedes
the admission boundaries recorded below. Fixed runtime shifts, helper-loop
shift/div/rem, and proven Integer peer normalization now use existing compiler
owners. `int_sqrt`, `fibonacci`, and `left_shift` were restored to natural source
forms and all three passed unchanged upstream testbenches in
`benchmark/runs/compiler-audit-natural/`. The following initial diagnostics and
workarounds are historical evidence, not current authoring instructions.

The initial ResBench port produced 43 compilable Python implementations and 13 explicit
unsupported entries out of 56 tasks. This note records source-authoring diagnostics
and the changes used to compile the implementations. Compilation is not a
functional, C++/Verilog simulation, synthesis, timing, or resource-score result.
The main benchmark runner owns those results.

Initial solutions were authored from the problem statement and the module
declaration, ending at its first `);`. Reference solutions, embedded RTL bodies,
expected outputs, and testbench implementations were not used for that initial
authoring. After the first functional run, the public testbenches for `mod_exp`
and `air_quality_index` were inspected to refine those two implementations, as
recorded below. No reference DUT implementation was inspected. The resulting
coverage is an assisted port result, not an unassisted model pass@1 score.

## Evidence and reproduction

The successful compile commands, exit codes, and output for all 43 solutions are
retained in
[`compile-results.json`](../../docs/gates/logs/resbench-author-check/compile-results.json).
The original per-task logs were overwritten by successful retries. To retain
negative evidence, five representative failed forms were reconstructed in a
separate ignored evidence directory and compiled again. Every reproduction exited
1 with the diagnostic recorded below. These reproductions do not modify the
benchmark solutions.

[`reproductions.json`](../../docs/gates/logs/resbench-port-findings/reproductions.json)
records each complete command, reproduction-source SHA-256, corresponding frozen
solution SHA-256, exit code, and log path. From the repository root, an example is:

```bash
.pycircuit_out/toolchain/install/bin/pycircuit compile \
  -c left_shift.py \
  --source-root docs/gates/logs/resbench-port-findings/src \
  -o docs/gates/logs/resbench-port-findings/units/left_shift --replace
```

The source and evidence directories must already exist. Logs are ignored local
artifacts; the tracked sources and this note do not depend on their presence.

All five diagnostic sites belong to the native source importer in
`compiler/lib/Compiler/PythonImport*`. The CLI reports `native source compiler
rejected the source`. The cited emitter code establishes that these are source
import/admission diagnostics while constructing MLIR. They are not evidence of a
Python capture failure, a later common-IR optimization failure, or a target codegen
failure. No compiler implementation change is included in this port.

## RB-SRC-01: conditional literal provenance at bitwise boundaries

**Tasks:** initially `mux4to1`; the related extension form affected `add_8bit`,
`subtract_8bit`, the five polynomials, `matrix_vector_mult`, `mse_loss`, `conv2d`,
and `power`.

The initial mux expression selected two Integer literals before combining the
result with a fixed-bit slice:

```python
bit0 = in_[:1] & (1 if sel == 0 else 0)
```

It emitted:

```text
loc("mux4to1.py":12:12): error: binding boundary requires authoritative source kind
```

The initial sign-extension expression selected a literal mask before OR:

```python
signed_a: ac.u9 = a
signed_a = signed_a | (256 if a >= 128 else 0)
```

It emitted:

```text
loc("add_8bit.py":14:16): error: unsigned boundary requires authoritative bits, Boolean or Integer provenance
```

Reproductions:
[`mux source`](../../docs/gates/logs/resbench-port-findings/src/mux4to1.py),
[`mux log`](../../docs/gates/logs/resbench-port-findings/mux4to1.log),
[`extension source`](../../docs/gates/logs/resbench-port-findings/src/add_8bit.py),
[`extension log`](../../docs/gates/logs/resbench-port-findings/add_8bit.log).
The rejection checks are in
[`PythonImportRecords.cpp`](../../compiler/lib/Compiler/PythonImportRecords.cpp#L1632)
and
[`PythonImportAggregates.inc`](../../compiler/lib/Compiler/PythonImportAggregates.inc#L1088).
They check source-kind/fixed-value authority. These checks alone do not establish
which earlier operation lost or intentionally omitted that authority.

**Source change:** the mux uses selector slices and AND/OR/NOT gates, as requested
by its problem. Sign extension keeps both conditional arms as fixed-bit values:

```python
signed_a = signed_a | 256 if a >= 128 else signed_a
```

**Classification:** authoring/admission boundary with an investigation opportunity.
The accepted spelling compiles. The evidence does not establish that the rejected
conditional-Integer spelling is required by the language, or that a provenance
propagation bug exists. A follow-up should reduce and trace the conditional value
before broadening admission. Any improvement must retain the source-kind checks.

## RB-SRC-02: runtime unsigned shift counts require explicit selection

**Tasks:** `decoder_2to4`, `left_shift`, `rotate_left`.

The initial expression was:

```python
return Output(out=in_ << shift)
```

It emitted:

```text
loc("left_shift.py":12:23): error: unsigned shift requires a proven static Integer count
```

Reproduction:
[`source`](../../docs/gates/logs/resbench-port-findings/src/left_shift.py),
[`log`](../../docs/gates/logs/resbench-port-findings/left_shift.log).
The check is in
[`PythonImportRecords.cpp`](../../compiler/lib/Compiler/PythonImportRecords.cpp#L3116).
The [language reference](../../docs/reference/language.md#L277) explicitly requires
a proven static, nonnegative Integer count; a runtime Bits count is outside that
contract.

**Source change:** `left_shift` uses three conditional stages with constant shift
counts 1, 2, and 4. `rotate_left` uses three corresponding constant-slice/concat
stages. `decoder_2to4` selects its four constant one-hot values explicitly.

**Classification:** documented supported-language limit. A future runtime-shift
operation could simplify these source expressions, but this port provides no
area, latency, compile-time, or simulation-time measurements supporting an
optimization claim. The explicit stages use the existing compiler route.

## RB-SRC-03: bounded-loop preflight excludes shifts and division/remainder

**Tasks:** `int_sqrt`, `log2_int`, `power`, `mod_exp`.

The initial square-root loop contained:

```python
for index in range(8):
    trial = root | (128 >> index)
    root = trial if trial * trial <= in_0 else root
```

It emitted:

```text
loc("int_sqrt.py":14:25): error: bounded for binary operation is unsupported
loc("int_sqrt.py":13:5): error: bounded for capability preflight failed
```

Reproduction:
[`source`](../../docs/gates/logs/resbench-port-findings/src/int_sqrt.py),
[`log`](../../docs/gates/logs/resbench-port-findings/int_sqrt.log).
The original `log2_int` used `1 << index`; `power` and `mod_exp` used
`in_1 >> index`. Each encountered the same binary-operation/preflight rejection.

The allowlist in
[`PythonImportBehavior.inc`](../../compiler/lib/Compiler/PythonImportBehavior.inc#L507)
admits Add, Sub, Mult, BitAnd, BitOr, and BitXor. It excludes shifts as well as
FloorDiv and Mod. This matches the bounded-loop profile in the
[language reference](../../docs/reference/language.md#L552).
`mod_exp` also requires remainder operations, so removing only its shift would
not put its original loop wholly inside that profile; no separate remainder-only
failure was measured here.

**Source change:** square root uses eight explicit trial stages; logarithm uses
fourteen explicit threshold selections; power uses three explicit exponent-bit
stages; modular exponentiation uses seven. The resulting operations are ordinary
pure helper expressions and compile without adding state or changing compiler
admission.

**Classification:** documented bounded-loop limit and source-maintainability
opportunity. Supporting more operations in bounded loops could retain compact
algorithmic source. This requires a separate semantic/admission review, budget
checks, and backend evidence; repeating source stages is not evidence of a
backend performance problem.

## RB-SRC-04: loop induction comparison needs an explicit fixed-bit conversion

**Task:** `fibonacci`.

The initial loop compared its fresh induction Integer directly with the u8 input:

```python
for index in range(127):
    next_value = previous + current
    previous = current if index < in_0 else previous
    current = next_value if index < in_0 else current
```

It emitted:

```text
loc("fibonacci.py":16:31): error: comparison operands require equal bits types
loc("fibonacci.py":14:5): error: bounded for reachable iteration 0 failed
```

Reproduction:
[`source`](../../docs/gates/logs/resbench-port-findings/src/fibonacci.py),
[`log`](../../docs/gates/logs/resbench-port-findings/fibonacci.log).
The equal-type rejection is in
[`PythonImportRecords.cpp`](../../compiler/lib/Compiler/PythonImportRecords.cpp#L3443).
The language reference identifies a fresh induction binder as an Integer; it does
not give that binder the fixed-bit type of an unrelated input.

**Source change:** declare `ordinal: ac.u8 = index` inside the pure helper's loop
and compare `ordinal < in_0`. This preserves the compact 127-iteration source loop
and passes compilation. It does not execute the helper during capture.

**Classification:** explicit type-boundary requirement. A diagnostic naming the
actual kinds/widths and suggesting a typed local could improve authoring guidance.
The evidence does not justify weakening Integer/Bits separation or asserting a
loop-provenance compiler defect.

## Invocation corrections, not compiler defects

Two early CLI attempts exited 1 before native source compilation:

| Diagnostic | Cause and correction |
| --- | --- |
| `pycircuit compile: publication parent does not exist` | Create the intended ignored evidence parent before publishing a source unit. |
| `pycircuit compile: [Errno 2] No such file or directory: '.../benchmark/solutions/benchmark'` | A relative `-c benchmark/solutions/resbench/name.py` was resolved under `--source-root benchmark/solutions`; use `-c resbench/name.py` instead. |

These were command construction mistakes. Their original terminal output was
observed, but standalone failure logs were not retained. They are not counted as
compiler capability failures.

## Scope left open

The 13 unsupported manifest rows record problem-information gaps: four FSMs omit
necessary transition/reset details, five pipelines omit exact reset/staging or
coefficients, three financial cases omit fixed-point details, and the Feistel case
omits its fixed round key/function. These are not demonstrated frontend, MLIR, or
codegen failures. This port did not infer an implicit clock domain or substitute
combinational output for sequential behavior.

No target codegen defect, optimizer regression, or measured optimization benefit
was established by the authoring compile checks. Functional failures and any
link/emit diagnostics from the main runner must be reported separately with their
actual candidate and test evidence.

## RB-SEM-01: signed remainder in modular exponentiation

**Task:** `mod_exp`. **Classification:** source-semantics correction after
consulting the unchanged public oracle; no frontend/MLIR/codegen defect shown.

The initial implementation selected nonnegative modular residues, an assumption
recorded in its source and manifest. The problem declares signed 8-bit inputs
and output, but does not explicitly distinguish a nonnegative mathematical
modulo from Verilog's signed remainder. The public testbench explicitly states
that `(-2)^3 % 5` must yield `-3`; the initial implementation returned `2`.

**Oracle-informed refinement:** compute the modular exponentiation of the base's
magnitude, then negate the residue when the base is negative and the exponent is
odd. The modulus uses its signed magnitude; a zero modulus and negative exponents
map to zero. This is a general parity/sign calculation, with no fixture-specific
input handling. Magnitude products remain u16 and residues at most 127 for the
signed 8-bit modulus domain. The original stimulus and expected values are
unchanged. The public testbench covers seven cases, including this negative-base
case and zero modulus; it does not establish exhaustive coverage of signed inputs.

| Evidence | SHA-256 |
| --- | --- |
| Original authored source | `5226d201c5bd82d645cc83bc088df256b9fd9dcaf7ea1850803685615a7ab05b` |
| Refined source before scalar-interface cleanup | `0238069a427a60dec22a2a456c3433a00bef334ae3b9cf2033804e186c995ef0` |
| Unchanged public testbench | `99020e4280f931568d1cda5150877ad13f9957558565ea6e8460fbb53500f01e` |

## RB-SEM-02: AQI coefficients use the oracle's Q0.8 approximation

**Task:** `air_quality_index`. **Classification:** problem precision ambiguity
resolved by the unchanged public oracle; no frontend/MLIR/codegen defect shown.

The problem gives decimal weights `0.5`, `0.3`, and `0.2` without specifying a
fixed-point encoding. The initial source represented them exactly as `5/10`,
`3/10`, and `2/10`, and truncated once after adding the weighted terms. For
`(123, 45, 67)`, that expression yields `floor(1220/10) = 122`.

The public testbench comments specify Q0.8 integer coefficients `128`, `77`, and
`51`, producing `(123*128 + 45*77 + 235*51) // 256 = 121`. The discrepancy is
coefficient quantization, not truncation of each term.

**Oracle-informed refinement:** use these coefficients for every input and divide
the complete numerator once by 256. Widen operands and the numerator to u18 before
multiplication/addition, then select the u16 output after division. This avoids
u16 intermediate overflow: the maximum possible numerator is
`255*(128 + 77 + 3*51) = 91290`. No stimulus, oracle arithmetic, or expected value
was changed. The original testbench exercises three cases; exhaustive equivalence
is not claimed.

| Evidence | SHA-256 |
| --- | --- |
| Original authored source | `2aefc1503338c55eaf553c608fff87d28f7c3a0a60e408b551ce97f49d4812e8` |
| Refined source before scalar-interface cleanup | `4dee2ad9756c348e24da3ab46f6c5cc61b4a1abac9f3eef4e03d9a203ffc701a` |
| Unchanged public testbench | `465e296566d14739a48e466dea4d5dd09999c2b3f4db6cb2e2427fbc9ecfb4af` |

## Refinement validation

```bash
python3 benchmark/run.py --suite resbench \
  --tasks mod_exp air_quality_index \
  --output benchmark/runs/resbench-oracle-refined-two --jobs 2
```

Both selected tasks passed the unchanged public testbench under Icarus. Each
completed compile, link, C++ emit, Verilog emit, Icarus compile, and simulation with
exit code 0. C++ was emitted but not simulated; FPGA resource scoring was not run.
This focused run selected two of 56 suite tasks and is not a fresh full-suite run.

Exact commands, source/testbench/final-IR/RTL hashes, and statuses are in
[`results.jsonl`](../runs/resbench-oracle-refined-two/results.jsonl). Original
simulation transcripts are retained for
[`mod_exp`](../runs/resbench-oracle-refined-two/mod_exp/simulation.stdout) and
[`air_quality_index`](../runs/resbench-oracle-refined-two/air_quality_index/simulation.stdout).
Before/after source hashes and testbench hashes are also retained in
[`hashes.json`](../../docs/gates/logs/resbench-oracle-refinement/hashes.json).

## RB-AUTHOR-01: remove single-field output wrapper Structs

**Tasks:** 40 single-output implementations. **Classification:** correction of
unnecessary author-written abstraction; not a product/frontend/MLIR/codegen bug.

The initial port wrapped each sole scalar output in a named `Output` Struct. The
user clarified that a Struct should combine multiple related fields; a single
Bits or Boolean result should be returned directly. The 40 single-field wrappers
were removed, and their pure `compute` helpers and `Top` Modules now return the
original field's exact scalar type. `bitwise_ops`, `matrix_vector_mult`, and
`conv2d` retain their related multiple-output Structs.

The manifest's output names/order remain the upstream Verilog ABI mapping. They
do not require the frontend result to have a wrapper Struct. The generic runner
owns adaptation from a scalar frontend result to that upstream output name.

The edit preserved inputs, widths, numeric expressions, and both oracle-informed
arithmetic refinements. An AST comparison checked each transformed source against
the old AST after removing only its wrapper declaration/construction and replacing
its helper/Module result annotations. Black then formatted all 43 files, and the
40 post-format ASTs matched that same expected transformation. No arithmetic-body
rewrite or compiler change was made.

Validation:

- Black 24.10.0 `--check benchmark/solutions/resbench`: all 43 files accepted.
- Seven representative independent source compilations passed: `mod_exp`,
  `air_quality_index`, `eq_comparator`, `fibonacci`, `polynomial_4`, `rotate_left`,
  and the retained multi-output `bitwise_ops`.
- Before/after scalar-source hashes, exact transformed-AST hashes, and compiler
  commands/results are retained in
  [`transform.json`](../../docs/gates/logs/resbench-scalar-outputs/transform.json)
  and
  [`compile-results.json`](../../docs/gates/logs/resbench-scalar-outputs/compile-results.json).

The earlier full-suite and two-task functional results above apply to their
recorded source hashes before this interface cleanup. A fresh full functional
run with the scalar-aware adapter is owned by the main runner. No runtime or
hardware-resource improvement is claimed from deleting these wrappers.
