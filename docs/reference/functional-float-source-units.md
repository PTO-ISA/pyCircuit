# Reusable functional numeric source units

## Main integration verification

PR322 integrates target main `775d0f68ce40733c8f197b59011373e4e330895b`
with a history-preserving merge, retaining the existing functional providers
and the upstream routed-anchor, transaction, availability and sibling-call work.
The importer uses explicit zero-inline-capacity branch-join storage, retaining
LLVM22 compatibility. The example catalog, CMake registry and navigation contain
all22 designs, including this functional System and four sibling examples.

The merged candidate passes the API gate25/25, standard examples gate13/13,
the functional System plus four sibling Systems5/5, and the focused functional
source generation/result-effects test1/1. System verification runs native
workers1/2 and RTL against the same final IR. Python unit checks finish with
822 passed/4 skipped; required changed-file pre-commit, API hygiene and strict
documentation checks pass. Formatting required by the existing gate preserves
all functional-provider/example ASTs. These are focused merge-integration
results, not a rerun of full nightly numerical or consumer release matrices.
Compiler tools are incrementally rebuilt using the existing LLVM22.1.8 SDK;
Verilator5.048 matches the repository's native-tool setup. Remote CI and review
remain separate observations recorded on the PR.

## Published provider and consumer evidence

Current computational source/test pin: `bc64496534588330e561904ec5c22c9a68e41420`.
Latest selected-release consumer test: `c389ac0c` (complete 802), with the
bounded matrix-default repair/follow-up at `3adf2e9f`. The earlier complete
799 at `3135bae7` and EXPDIF follow-up at `c9ac6d53` remain separate.
The [latest consumer report](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/latest_release_20261021.md)
requires SSM repository access. Earlier compatibility/frozen measurements
below remain historical.

The subsequent [latest802 remaining18 diagnostic](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/remaining18_bfi_20261024.md)
uses the actual `ops-20261021` inputs, separate from newcc799. Each of the18
previously unconfirmed inputs first differs at HL.BFI: the independent accepted
ASL bit-loop and PYC produce `0x200000002`, while OFF produces `0x2`. Including
the already sealed current index139 proof gives19 first-position attributions,
not19 whole-program correctness proofs or release passes. The diagnostic and
actual raw/source-binding evidence belong to SSM and require repository access.
No public provider, source-capture, compile/link, C++/RTL or runtime behavior is
changed; sealed public numerical/state gates are not repeated for this report.

pyCircuit publishes reusable computation as typed, source-captured Python
functions and `@ac.module` providers. These examples use the public
`pycircuit compile` → explicit `link` closure → verified final IR → `emit`
commands. C++ and Verilog are generated from the same linked artifact. No
Python design function is executed to calculate DUT results.

The [functional numeric example](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/README.md)
contains inspectable algorithm providers and a reproducible
[`build_units.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/build_units.py) driver.
They exercise existing compiler and runtime capabilities; they are example
algorithms, rather than new built-in floating primitives or a complete IEEE
library. Imports refer to published source-unit interfaces. Link receives the
complete body closure, including the nominal response types.

## Public provider inventory

Each top below is prefixed with `functional_float.`. The source directory is
[`examples/functional_float`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float).

| Source / top suffix | Supported computation |
| --- | --- |
| `fp32_product.FP32Product`, `fp32_sum.FP32Sum` | Binary32 multiply/add, RNE, subnormals, canonical NaN and signed zero, status fields |
| `fp32_product_sum.FP32ProductSum` | Product rounded to binary32, then independently rounded Sum; union of both stages' effects, final-result zero |
| `fp32_exp.FP32Exp` | Binary32 approximate exp: ln(2) reduction, sixth-order Horner polynomial, separately rounded operations and final power-of-two scaling |
| `fp32_divide.FP32Divide` | Binary32 quotient, normalized widened integer division, remainder sticky and RNE; reciprocal is numerator one |
| `fp64_product.FP64Product`, `fp64_sum.FP64Sum`, `fp64_divide.FP64Divide` | Binary64 multiply/add/quotient using `ac.bits[128]`, RNE, subnormals and special values |
| `float_sqrt.FloatSqrt` | Binary32/binary64 RNE square root from 55 restoring integer-root steps, subnormal and IEEE special-value handling |
| `float_compare.FloatCompare` | Binary32/binary64 eq/ne/lt/ge, explicit quiet/signaling policy; all comparisons false for NaN |
| `float_to_integer.FloatToInteger` | Binary32/binary64 to signed/unsigned 8/16/32/64-bit integers; RNE/RTM/RTP/RTZ/nearest-away, saturated invalid endpoints and NV/NX |
| `float_to_integer_bits.FloatToIntegerBits` | Binary32/binary64 to dynamic signed/unsigned width1..64; seven rounding modes, finite-overflow OF\|NX, wrapping or saturation; separate endpoint/NV special policy |
| `float_round_integral.FloatRoundIntegral` | Binary32/binary64 integral-valued rounding, seven explicit policies, signed zero, NaN/Inf and large already-integral values |
| `packed4_encode.Packed4Encode`, `packed4_decode.Packed4Decode` | Complete16-code E2M1/E1M2 maps, RNE encode, explicit endpoint policy; raw-value codec only |
| `finite_lowp_cast.FiniteLowpCast` | One-round finite binary64 to E4M3/E5M2/HiF8 within each maximum finite magnitude; RNE plus HiF8 RHB, explicit domain rejection |
| `float_from_integer.FloatFromInteger` | Signed/unsigned 64-bit integers directly rounded once to binary32, binary64, binary16 or bfloat16 (format selector 0/1/2/3) |
| `fp32_max.FP32Max` | Binary32 maximumNumber/minimumNumber, signed-zero tie and signaling-NaN effects |
| `fp32_to_fp16.FP32ToFP16`, `fp32_to_bf16.FP32ToBF16` | Binary32 to binary16/bfloat16 conversion and effects |
| `input_expand.InputExpand` | Exact binary16/bfloat16 raw expansion to binary32 |
| `word_division.WordDivision` | Signed/unsigned 32/64-bit quotient with explicit zero-result or all-ones zero-denominator policy; generic WordArithmetic defaults remain unchanged |
| `fp32_remainder.FP32FloorModulo` | Divide-RNE, floating floor, Product-RNE, then Sum-RNE; explicit positive-zero divisor-zero policy |
| `word_arithmetic.WordArithmetic` | Wrapping 32/64-bit add/subtract/multiply, bitwise logic, shifts, signed/unsigned divide/remainder; word results sign extend |
| `bit_transform.BitTransform` | Rotating signed/unsigned extraction, wrapping inclusive insertion, CLZ/CTZ/popcount, select and predicates |
| `integer_element.IntegerElement` | Unsigned wrapping 16/32-bit arithmetic, sequence and copy; division/remainder reject zero divisor |
| `address.RowMajorAddress` | Byte address and valid-coordinate predicate for bounded row-major transport |
| `fp32_u32_add.U32Add` | Wrapping unsigned 32-bit addition |

The word provider's defined zero-divisor policy is quotient zero and remainder
dividend. It differs deliberately from `IntegerElement`, whose zero-divisor
request is rejected. Select the provider whose documented contract is needed.
`BitTransform` selectors 0–6 and 8–15 are defined; selector 7 and widths outside
1–64 reject. None of these selectors is an ISA instruction encoding.

## Value and effect boundary

Floating responses group the result with invalid, overflow, underflow, inexact
and zero. Quotient and binary64 word responses additionally carry
divide-by-zero. ProductSum retains effects from both rounded stages and always
requests Sum with the original request predicate; it never substitutes an FMA.
For an active request, a canonical IEEE-invalid result is represented by `invalid=1` and a known NaN;
the arithmetic leaves clear `valid` for that outcome. An IEEE consumer can
commit the defined NaN and NV after checking the whole response. A response
with neither `valid` nor `invalid` authorizes no numerical commit. Request
availability, IEEE invalid, and an unknown runtime wire are distinct conditions.
A false input request never authorizes a numerical commit, even when a
numerical status field is independently asserted. Consumers must inspect the actual response contract rather than treating all
false `valid` values as success or replacing unknown data with zero.

The generated runtime adapter assigns known typed input wires, evaluates Work,
checks the complete output wire, and reads its value/effects. State updates,
memory Store ordering and fault policy remain with the consumer. The examples
have no ELF loader, instruction decoder, tile scheduler or architectural state.
Those belong in repositories such as SuperScalarModel.

## Reproduce from public source

Use the [CompilerDev build and install](../getting-started/installation.md) and
[public source-unit workflow](../development/source-unit-workflow.md). From a
checkout of the PR branch with its tools configured:

```bash
PYTHONPATH=python python3 examples/functional_float/build_units.py \
  .pycircuit_out/functional_numeric --replace
# Or generate just one top and its complete import closure:
PYTHONPATH=python python3 examples/functional_float/build_units.py \
  .pycircuit_out/functional_quotient --only fp64_divide --replace
```

The driver invokes public commands; it supplies no numerical implementation.
`units/` holds independent publications, `linked/` holds final designs, and
`cpp_*`/`verilog_*` hold target outputs. Generated products stay untracked.
Compose providers in a source module and explicitly link their closure when
one verified artifact is desired. Separate final C++ artifacts should use
separate translation units: each emitted root owns its support header and
nominal-type declarations.

## Verification and limits

The existing nightly API entry includes
[`test_functional_float_source.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/tests/system/test_functional_float_source.py)
and [`test_functional_numeric_source.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/tests/system/test_functional_numeric_source.py).
They cover compile/link and both emitters; generated native/RTL ProductSum
exception propagation and separate rounding; 5,852 exact integer/rational
raw-and-status vectors for binary64 arithmetic, four integer-conversion formats
and quiet/signaling comparisons; 5,856 floating-to-integer raw-and-status vectors over both formats, five rounding modes and eight integer targets; 1,060 integer-isqrt midpoint reference vectors for square root; and 4,112 bit-loop reference cases on native
and RTL. Native drivers compile with `-O1`. The example build driver also
compiles, links and emits every published top; emission alone is not numerical
execution coverage of every provider.

The exp polynomial is an approximation, not correctly-rounded transcendental
certification. Other arithmetic rounding modes, exhaustive IEEE inputs,
a complete ISA are outside this example's claim. The
SSM release ledger (476 fixed inputs, including 439 historical PASS and 37
original FAIL) is consumer evidence and remains separate from these numerical
oracles. Its current OFF/PYC results and exceptions belong in the SSM report;
a historical 439/439 does not certify the current branch.

The consumer's subsequent [43-target first-cause audit](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/frozen_failure_causes.md)
keeps those historical targets open and the original37 failures separate.
It binds actual frozen encodings to accepted ASL and distinguishes earlier
consumer admission gaps from incompatible scale/auxiliary layouts, logical
row indices and missing PE participants. SSM now also reuses the existing
generated `BitTransform` provider for raw TGATHER/TSCATTER row selection,
with its own36-profile encoded consumer test across four PEs (144 accepted
PE executions). This adds no pyCircuit ISA
decoder, no new provider semantics and no new public numerical-oracle claim.
The complete396 raw matches remain the sealed measurement at SSM9b0af112;
the directed extension at71c636f7 does not promote its two invalid frozen
inputs to PASS. SSM is private and its report/fixtures require repository
access; this public example and all provider sources remain independently
available. Public provider validation remains atb9f3da19; this documentation
continuation does not rerun the sealed API/native/RTL gates.

The [SuperNPUBench235 evidence supplement](https://github.com/PTO-ISA/SuperNPUBench/issues/235#issuecomment-6307023978)
reports only24 proven descriptor/index incompatibilities in ops-20260916,
with accepted-ASL links and minimal reproductions. PYC capability gaps and19
participant/configuration/compatibility cases are not attributed to Bench ELF
bytes. That consumer report does not expand this provider's numerical scope.

The subsequent [current release measurement](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/current_release_20261021.md)
uses actual published
[SuperNPUBench ops-20261021-newcc](https://github.com/PTO-ISA/SuperNPUBench/releases/tag/ops-20261021-newcc)
assets and a rechecked accepted PTO0.59.0 /publication0.59.0.0 snapshot at
pto-specb0165320. Its799 inputs and official PE policies are independent of
the old476 freeze:415 common case names,384 added,61 absent; case names do
not certify equal recompiled bytes or transfer an old PASS.

SSM extends its own consumers of the existing Word/Bit, Product/Sum and
negative-power providers: Local TIMG2COL has12 four-PE/two-generation native
profiles and51,072 raw/Store matches; Shared CScale has9 profiles/6,168 matches;
fused Shared C/D and late scalar parameters have9 profiles/4,608 matches;
raw scalar fill has19 profiles/2,292 matches. Canonical THREAD_ID and HL.BFI
field bindings receive separate native checks. These remain SSM instruction,
descriptor, scheduling and state/effect work; they do not add pyCircuit ISA
decoding or a full precision/ISA library. At that earlier compatibility stage, core provider code and its existing
C++/RTL artifacts were pinned atb9f3da19. The later exact RTZ extension at
39ff674 and the post-admission799 result are documented separately below;
unchanged core oracles and SDK gates were not replayed.

At consumer test head98ed0baa, all799 inputs were measured with independent
OFF/ON outputs,180seconds/path and eight workers. OFF has761 rc0/R2=0,
20 errors and18 timeouts; PYC has571 rc0/R2=0,226 errors and2 timeouts.
Observed raw is479 MATCH,87 DIFFER,233 NOT_RUN. Failed paths retain UNKNOWN
R2. Canonical identity and HL.BFI model-contract differences are recorded
without filtering GPR/memory bytes or choosing an old arithmetic fallback.
The old frozen439 result remains396 matched/43 failed, with original37
separate. These are consumer/profile observations, not a799/799 or all-ASL
claim for this public example library.

The [new-release Bench235 follow-up](https://github.com/PTO-ISA/SuperNPUBench/issues/235#issuecomment-6308383132)
records actual new bytes before assigning defects: the old vector/RowMaxIn
layout issues no longer apply; two row-index inputs and one explicit SYS
envelope sample remain release-ELF issues. PYC gaps and model-side differences
are excluded from that attribution. The consumer report keeps rc/R2/raw
results, legacy model policies and accepted-ASL admission distinct; R2=0 or
raw equality alone is not complete ASL or kernel-lifecycle certification.

## Consumer-selected arithmetic policies

`WordDivision` exposes `zero_ones` rather than changing WordArithmetic's
published zero-quotient default. Both zero policies, signedness/word width and
inactive requests have 2,320 independent native/RTL raw-and-valid matches.
A consumer can select the policy its architectural model actually declares.

`FP32FloorModulo` models a specific, separately rounded sequence. It first
rounds division to binary32, floors that floating quotient, rounds its product
with the divisor, then separately rounds subtraction. A zero divisor returns
positive zero. It is not an exact-real modulo with a single final rounding;
intermediate overflow and rounding remain visible. Its canonical NaN/effect
outputs are deterministic and do not depend on a host floating-point library.
The independent staged rational oracle checks 1,348 raw/status cases on native
and RTL. Existing ProductSum and matrix multiply/add paths keep their separate
rounding; this provider introduces no FMA replacement.

The earlier paired-release SSM consumer selected its floor sequence and
all-ones scalar zero quotient. The referenced PTO ASL instead declares a truncating
floating remainder and zero scalar quotient. These differences are explicitly
consumer policy and are not an ISA-conformance claim for this example library.
The user chose to preserve SSM floor behavior for the paired release experiment;
neither the ISA/ASL nor the ordinary OFF arithmetic is modified by the example.

The latest executable floating TREM/TREMS reference uses RTZ although related
prose still says floor; integer Tile remainder remains floor. The separate
SSM1051 gfrun task is a future dependency, not part of this example change.
The earlier paired experiment used the named floor compatibility sequence.
The subsequent ASL-led consumer repair selects the separately published RTZ
provider for floating TREM/TREMS; integer Tile floor remains independent.

## Direct floating format conversion

[`FloatConvert`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/float_convert.py) accepts a raw
64-bit carrier and source/destination selectors: binary32=0, binary64=1,
binary16=2, bfloat16=3. All 16 pairs perform one RNE step from the original
significand, including subnormal inputs and post-rounding tininess. The
[`float_formats.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/float_formats.py) helpers
provide shared unpacking and widened integer packing; an intermediate binary32
cast is not used for binary64 to either 16-bit destination. NaNs become the
positive canonical quiet NaN of the destination; signaling NaNs assert NV.
Signed zeros and infinities retain their signs. Overflow, underflow and
inexact describe the conversion, rather than a host floating-point cast.

The public numeric source test checks 8,416 independent exact-rational raw and
status vectors in generated native and RTL execution, including midpoints below
minimum normal and inputs that distinguish direct rounding from two casts.
Generate it with `build_units.py --only float_convert` using the same workflow
above. This bounded evidence does not establish exhaustive correctly-rounded
coverage of all floating operations.

## Explicit fused multiply-add

[`FloatFused`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/float_fused.py) computes one-round
RNE `lhs * rhs + addend` for binary32/binary64, selected by `wide64`. It uses
an exact widened significand product, normalized alignment with retained sticky
information, and one final pack. The 5,508 independent integer/rational cases
check native and RTL raw results and all status fields, including cancellation,
subnormals, infinities, NaNs and cases distinguishing fused from separate
rounding. Generate it with `build_units.py --only float_fused`.

This is an explicitly selected algorithm for consumers that request a fused
operation. `FP32ProductSum` and ordinary matrix consumers continue to execute
Product-RNE followed by Sum-RNE; neither invokes this provider. No compiler
fusion of ordinary multiply/add expressions is introduced by this example.

## Four-format fused provider and logarithm approximation

[`FloatFusedFormat`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/float_fused_format.py)
extends explicit one-round multiply-add to the same four format selectors as
FloatConvert. The raw carrier is 64 bits; binary16/bfloat16 results round
directly to their destination, rather than through binary32. Its exact-integer
reference checks 11,016 native/RTL raw-and-all-status cases, including all
earlier binary32/binary64 cases and 16-bit cancellation/subnormal inputs.
The existing `FloatFused(..., wide64, ...)` interface is preserved.

[`FP32Log`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/fp32_log.py) normalizes the raw
binary32 significand, applies a 25-term atanh expansion with Q80 fixed-point
integers, and combines the exponent contribution using a fixed Q80 ln(2).
[`fp32_log_step.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/fp32_log_step.py) holds
the pure repeated step and exact partial-product helper. Both units fit the
current source evaluator's constant/slot budgets without raising them. The
independent Decimal100 reference checks 1,043 native/RTL raw/status vectors,
including adjacent values around one, subnormals, infinities, NaNs and domain
errors. Zero returns negative infinity with DZ; negative nonzero inputs return
canonical quiet NaN with NV. This is a bounded approximation with evidence on
the recorded vectors, not an exhaustive correctly-rounded logarithm claim.

Generate the selected closures with `build_units.py --only float_fused_format`
or `--only fp32_log`. These source providers introduce no instruction decoder,
ELF policy, consumer state or host numerical fallback.

## Binary32 boundary corrections

Equal negative addends retain their common sign (for example -1 + -1 = -2).
Product and Sum detect tininess after rounding: an inexact value that rounds
to the minimum normal result asserts NX without UF. Directed native/RTL
ProductSum cases cover both corrections while retaining two separate rounds
and the union of stage effects.

## Finite 19-bit parameter format

[`FP19FiniteExpand`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/fp19_expansion.py) expands
a sign/E8/M10 scalar to binary64 exactly. All256 exponent codes are finite,
including255; code0 includes subnormals and signed zeros. The scalar result
uses no one-field wrapper or ISA-specific descriptor. This explicit finite
format differs from IEEE special-value encodings and is useful for compact
scale parameters. All524,288 raw inputs match an independent exact-rational
reference in native execution;5,632 RTL inputs cover both signs, every exponent
with boundary fractions and deterministic random cases.

Use `build_units.py --only fp19_expansion`; the same numeric source test owns
its compile/link/dual-emit and execution oracle. This exact expansion does not
certify the surrounding consumer's quantization pipeline.

## Encoded negative binary32 powers

[`FP32NegativePower`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/fp32_negative_power.py)
constructs the raw RNE binary32 value2**(-code) for unsigned codes0..254.
Code255 selects canonical quiet NaN. Codes127..149 produce subnormals;
codes150..254 round to zero. The constructor returns a scalar raw word;
arithmetic status belongs to a subsequent consumer of that encoded scale.
All256 codes match an independent exact-rational reference on native and RTL.
Use `build_units.py --only fp32_negative_power`. This numerical convention is
separate from the finite19 format above and adds no ISA decoding policy.

The reusable `lowp_decode.LowpDecode` and `lowp_encode.LowpEncode` examples
provide integer-only E4M3FN (kind0), E5M2 (kind1) and HiF8 (kind2) codecs.
Decode expands every raw byte exactly to binary32, with canonical special
values. Encode accepts binary32 and returns raw bytes plus effects. FP8 uses
RNE; HiF8 also supports RHB ties toward positive infinity and explicit finite
saturation. E4M3FN clamps overflow to its largest finite value without OF;
HiF8 reports tininess after rounding, retaining NX without UF when a tiny value rounds up to the minimum normal. These example conventions are
explicit policies, not a claim of a complete low-precision arithmetic library.
`lowp_formats.py` derives adjacent HiF8 values from exponent bins, and
`float_formats.pack_float` shares the integer RNE packing algorithm.
The independent rational oracle in `tests/system/functional_lowp_oracle.py`
checks every decode byte and representable values, midpoints with adjacent
binary32 values, signed boundaries, special values and deterministic random
encodes. Both generated targets consume the same linked artifact.

`e8m0_encode.E8M0Encode` is a separate positive exponent-only scale example.
Its seven exponent-rounding modes (nearest-even, floor, ceiling, toward-zero,
nearest-away, odd, hybrid) use exact integer comparison with the geometric
midpoint. Zero, negative and NaN sources return known FF and NV; out-of-range
exponents retain OF/UF/NX with explicit saturation selection. It accepts raw
binary32; half/BF16 consumers can first use exact expansion. The independent
rational native/RTL oracle checks35,840 raw/status vectors. This scale policy
is separate from HiF8 and `FP32NegativePower` sentinel conventions.

Decode kind3 additionally expands every E8M0 byte: code0 is positive2**(-127),
1..254 are powers of two and255 is canonical quiet NaN. The four-format decode
oracle covers2,048 native/RTL raw/status requests, including inactive requests.
The LowpEncode kind3 selector remains unsupported; use E8M0Encode for scales.

## Exact RTZ-quotient binary32 remainder

[`FP32TruncModulo`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/fp32_trunc_modulo.py)
adds a reusable numerical policy separate from `FP32FloorModulo`. Its public
entry takes `lhs`, `rhs` raw binary32 and `request_valid`, returning
`FP32Response`. Integer modular exponentiation computes the exact remainder
without an intermediate rounded quotient. Finite nonzero results are exactly
representable, including subnormals; exact cancellation is +0, while a zero
input preserves its sign. Infinity as dividend or zero as divisor is invalid;
quiet/signaling NaNs follow the explicit canonical-NaN/status contract.

The current accepted ASL snapshot has a real owner conflict:
[TREM's accepted contract](https://github.com/PTO-ISA/pto-spec/blob/b0165320a4b575b18d2f4d93faf40178f661ab49/asl/tile/elementwise-tile-tile/transcendental/TREM.asl)
says floating floor, while
[ReferenceTileFloatingModulo](https://github.com/PTO-ISA/pto-spec/blob/b0165320a4b575b18d2f4d93faf40178f661ab49/asl/tile/model/numeric/reference-conversion.asl#L238)
uses an exact-real RTZ quotient and final encoding. For -7 and +3 these give
+2 and -1 respectively. This new provider implements the executable RTZ policy;
it does not resolve or edit that ASL conflict. The existing floor provider is
a named compatibility policy, not proof of conformance to the executable
floating definition. Integer Tile floor semantics are independent.

`tests/system/test_functional_numeric_source.py::test_fp32_trunc_modulo_native_and_rtl`
and `tests/system/functional_trunc_modulo_oracle.py` publish the minimum
reproduction. Compile/link and both emits completed with the existing SDK;
1,350 independent Fraction vectors matched native and RTL raw/status. No
compiler, language admission, runtime or public floor algorithm was changed.
The consumer repository owns which policy is bound to each instruction.

## ASL-led consumer repair evidence

The public numerical source pin for the exact RTZ extension is
`39ff674cefee19cd7bfb4502ea64a5732805ee44`. Current source compilation,
explicit linking, C++/RTL generation and the1,350-vector independent oracle
passed with the existing SDK; there is no SDK/compiler/runtime rebuild or
new precision/tolerance policy.

The consumer continuation distinguishes ownership instead of using OFF as an
oracle. Shared TLOAD logical width/packed-lane addressing, Local M16/M32
arithmetic and reduction pitch, CUBE conversion binding and Local source
subviews belong to SSM. The public Product and BitTransform units already
returned the correct independent expectations. Those consumer repairs do not
establish a pyCircuit compiler/API defect. Binding a documented staged floor
provider to a floating instruction whose executable reference uses RTZ was
also a consumer policy mismatch; the new public exact provider fills that
numerical capability without changing the floor provider's contract.

At SSM test commit `e8a353cb`, an affected-only136-row measurement used the
same latest release, official PE,180-second paths and eight workers: OFF136
rc0/R2=0; PYC4 rc0/R2=0/raw MATCH (the four floating remainder fixtures),
132 rc2/R2 UNKNOWN/raw NOT_RUN. The four matches were existing release
completions, not four new passes. Subsequent source-subview work has separate
consumer tests and measurements in the
[SSM current-release report](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/current_release_20261021.md)
(private repository; access required). None of this overwrites the sealed
799-row479 MATCH/87 DIFFER/233 NOT_RUN result, or the older476 ledger with
historical439396 matched/43 failed and its separate original37 category.

Raw DIFFER is not itself an ASL verdict. For example, the actual latest
release cases `timg2col_fp32_64x8.elf` and
`kernel_matmul_matmul_lowp_FP8_B1_M64_N128_K7168_tM128_tN128_tK1024.elf`
execute SSRGET0x26 in STD blocks, whereas the
[accepted SSRGET owner](https://github.com/PTO-ISA/pto-spec/blob/b0165320a4b575b18d2f4d93faf40178f661ab49/asl/scalar/sys/SSRGET.asl)
requires SYS before effects. Compatibility-mode PE-identity output differences
on those instructions cannot certify either whole ELF as ASL compliant. The
remaining DIFFER rows retain their owner-specific or UNKNOWN status; they are
not all pyCircuit defects, OFF defects or Bench defects.

### Complete post-repair release measurement

SSM source/test commit `cc7289f0fd99ee8517a8f7dfe282849ec9de3fb3` completed the
one necessary post-admission799 run with the same release, official PE,
180 seconds/path and8 workers. OFF has761 rc0/R2=0,20 rc1 and18 timeouts;
PYC has376 rc0/R2=0 and423 rc2. R2 is UNKNOWN for38 OFF and423 PYC paths.
Raw comparison is354 MATCH /19 DIFFER /426 NOT_RUN, with no skipped inputs.
The [complete per-input result](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/release_ops_20261021_newcc_asl_results.json)
requires private SSM access. This is observed process/raw verification, not
whole-ISA ASL certification. Of the earlier87 raw differences,68 now reject
before completion and19 remain DIFFER; the68 are not repaired numerical
matches. The older compatibility measurement and frozen476 remain unchanged.

The consumer also corrects its own SSRGET admission: supported SYS is required
before effects; only the implemented accepted THREAD_ID0x26 returns a value,
and other selectors explicitly reject instead of fabricating zero. Independent
checks preserve destination and Store state on rejection. Of341 domain first
rejections,302 have actual executing-PC plus STD0x0800/SSRGET0x26 byte evidence
in this release;39 remain attribution-UNKNOWN. Only confirmed release defects
belong to [Bench235](https://github.com/PTO-ISA/SuperNPUBench/issues/235#issuecomment-6308383132).
Missing generated consumers remain SSM/pyCircuit capability work, including
mixed Local/Shared ACC schemas, MX scales/types and remaining conversion or
postprocess modes; their first rejection is not evidence of an ELF defect.

The new public numerical code remains the independently tested exact RTZ
provider at39ff674; no compiler/runtime/API defect was inferred from those
consumer binding or admission failures. No remote CI success is inferred from
local oracle or native evidence.

## Concentrated conversion and consumer repair

The additional source examples
[`float_to_integer_bits.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/float_to_integer_bits.py)
and [`finite_lowp_cast.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/finite_lowp_cast.py)
use the same public source capture, separately compiled provider closure,
verified final IR, and C++/RTL emission as the inventory above. They add example
algorithms; the compiler and SDK do not gain hidden floating primitives.
Their explicit request domains and selector policies are described in the
[example README](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/README.md).

The focused tests live in
[`test_functional_numeric_source.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/tests/system/test_functional_numeric_source.py),
with independent expectations in
[`functional_integer_bits_oracle.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/tests/system/functional_integer_bits_oracle.py)
and [`functional_finite_lowp_oracle.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/tests/system/functional_finite_lowp_oracle.py).
The new gates initially had2,944 and3,033 native/RTL raw-and-status matches respectively.
The affected HiF8 encoder regression has20,348 such matches and checks that
rounding to the minimum normal clears UF while retaining NX. Existing
`FloatToInteger` endpoint/NV behavior and separated ProductSum rounding remain
their own contracts. A consumer selects a provider and translates its policy
according to its architectural owner; successful generation alone does not
establish ISA correctness.

SSM repairs in this batch belong to its consumer: ordered Local/Shared C/A/B
and parameter roles, packed FP19 quantization descriptors, final single-round
conversion, RowMaxInit, exact CScale, and cooperative publication scheduling.
They are not pyCircuit API defects. The documented conversion examples are
public and reusable independently of the private SSM repository. Consumer
release measurements and their current limits are recorded separately below
and in the SSM documentation, with access to that repository required.

The same public builder now exposes `float_round_integral.FloatRoundIntegral`,
`packed4_encode.Packed4Encode`, and `packed4_decode.Packed4Decode`; their
source/helper files are beside the other examples. The first composes numeric
providers to round to integer-valued binary32/binary64 while retaining signed
zero and large integral values. The latter implement the complete16-code
E2M1/E1M2 value maps with RNE encode and an explicit overflow-endpoint input.
These are reusable algorithms with declared policies, separate from a
consumer's Tile packing, opcode, layout, lifetime and rounding admission.

[`functional_round_packed_oracle.py`](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/tests/system/functional_round_packed_oracle.py)
provides independent rational expectations. The registered source tests check
C++ and RTL from the same final artifacts:4,340 integral-rounding,2,448 packed
encode and64 packed decode result/status cases. The additional RHB selector
in `FloatToIntegerBits` expands that provider's focused gate to3,392 cases.
No compiler/runtime SDK rebuild or full IEEE/ISA certification is implied.
See the [example policy details](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/README.md).

## Concentrated batch result and ownership

The current public computational source isbc644965. The final focused source
checks pass in both native C++ and RTL from the same linked IR: integer-bit
conversion3,392; integral rounding4,340; packed encode2,448/decode64. The
unchanged finite-lowp gate has3,033 cases and affected HiF8 encode20,348,
recorded at3f15fca7 (those files are unchanged inbc644965). Selected changed
Python sources pass Ruff and `git diff --check`. Whole API/native SDK checks
are earlier23/23 and3/3 evidence, not repeated final-candidate certification.

The private SSM consumer repairs cover mixed Local/Shared ACC schema, MX
ScaleA/ScaleB roles and executable Word arithmetic policy, exact CScale,
postprocess mode/descriptor ABI, one-round final casts, all13 Local binding
forms and source lifetime, numeric status at publication, predicate/index
consumers, packed transport and typed EXPDIF. Generated numerical providers
remain the public dependency; no private bundle, hostfloat or old runtime is
needed. Public compiler/runtime implementations are unchanged in this batch.

The separate newcc799 run at3135bae7 uses accepted ASLb0165320 (0.59.0,
publication0.59.0.0), official PE and180 seconds/path with8 workers:
OFF761 rc0/R2=0,20 rc1,18 timeouts; PYC384 rc0/R2=0,414 rc2,1 timeout.
Raw observations:364 MATCH /19 DIFFER /416 NOT_RUN. UNKNOWN R2:OFF38/PYC415.
All19 DIFFER cases have individually recorded first-divergence BFI evidence:
ASL/PYC0x200000002 versusOFF0x2 for first32,last63,lhs=rhs2. This proves the
first divergence, not all later computation. Equality is not an ASL oracle.

The final c9ac6d53 consumer follow-up repairs a missing FP16/BF16 SUB rounding
before EXP, demonstrated by four legal profiles/128 raw values and Stores.
The public FP32Exp algorithm is unchanged. Its full-response memo has96 direct
provider binding checks. The14 opcode-selected release cases yield2 MATCH and
12 NOT_RUN (OFF8 rc0/6 rc1, PYC2 rc0/12 rc2, R2 unknown6/12); former timeout442
reaches invalid STD/SSRGET in45.251 seconds. The full799 result is not rewritten.
The [consumer report](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/repair_batch_20261021.md)
and complete/slim ledgers require SSM access. Old476/439 remains396 matched /
43 failed, original37 separate. No full-release, whole-ISA or full-precision
acceptance is claimed. Remote CI is NOT RUN/SKIPPED, not inferred green.

This fixed-input batch measuresops-20261021-newcc (799). A final live API read
also observes newer [ops-20261021](https://github.com/PTO-ISA/SuperNPUBench/releases/tag/ops-20261021)
at2026-10-21 07:20:15 UTC (802 ELFs, different compiler); those assets are
NOT RUN here. The measured release is not presented as GitHub's latest assets.
Only separately proved release-ELF violations are tracked in
[SuperNPUBench#235](https://github.com/PTO-ISA/SuperNPUBench/issues/235).
Unmapped output-pointer invocation ownership remains UNKNOWN and is excluded.

The concentrated release-ELF-only supplement and exact field/owner evidence
are [recorded in Bench#235](https://github.com/PTO-ISA/SuperNPUBench/issues/235#issuecomment-6318860003). It covers only
ops-20261021-newcc; the separate hosted-entry discussion is unchanged.

## Fixed latest Bench release and ASL validation

The consumer now actually validates the separately selected
[ops-20261021 release](https://github.com/PTO-ISA/SuperNPUBench/releases/tag/ops-20261021),
published 2026-10-21 07:20:15 UTC, against accepted ASL
`b0165320a4b575b18d2f4d93faf40178f661ab49` (architecture 0.59.0,
publication 0.59.0.0). These versions were selected once for the run. Both
actual archives were downloaded: 428 microbenchmark and 374 one-level ELFs,
802 total. All 799 common names with newcc were executed using the new assets;
three additions and no removals are recorded. Official source policy gives
401 PE1 / 401 PE4; the added matmul and both RMSNorm shapes require four PEs.
No input-hash list, SDK rebuild or repeated sealed provider oracle is needed.

The release reports a different producer compiler: LLVM 8610ef1c7 / TileOP
8f43777, compared with newcc's 084bbc311 / 0150baa. Those are release-reported
production versions, not later branch heads used as original provenance.
The release's comparison against ops-20261020 does not replace this newcc
inventory comparison or any actual numerical execution.

At SSM test commit c389ac0c, the complete 802 run has OFF 772 rc0/R2=0,
12 rc1 and 18 timeouts; PYC 384 rc0/R2=0 and 418 rc2; terminal R2 UNKNOWN
30/418. Raw observations are **364 MATCH / 19 DIFFER / 419 NOT_RUN**, with
no skips. Every path retains 180 seconds and the runner has eight workers;
CLI defaults are unchanged and the existing explicit step guard avoids an
extra 1M-step cutoff. Raw scope remains all 24 GPR words per PE plus writable
ELF PT_LOAD bytes, excluding heap-only outputs; equality is not an ASL oracle.

The new assets fix the earlier bad explicit GEMV M field by omitting LB0.
Accepted descriptor-state.asl defaults absent dimensions to one. The SSM
consumer had incorrectly used zero: this is an integration descriptor defect,
not a pyCircuit public numeric defect. Fix 3adf2e9f initializes matrix M/N/K
to one at block entry; explicit zero still overrides the default and rejects.
The independent native contract passes 42 profiles across four PEs, with
120 nonzero raw values / 120 actual Stores, plain/bias/ACC TMATMUL/TGEMV,
explicit and omitted dimensions, zero-Store rejections and sentinels.
No public model, compiler, generated Runtime or numerical algorithm changed.

Seven affected/control actual release cases were separately rerun at3adf2e9f:
OFF 7 rc0/R2=0; PYC 4 rc0/R2=0 and 3 rc2/UNKNOWN; **4 MATCH / 3 NOT_RUN**.
Only two cases add matches; three valid GEMV bindings now reach their distinct
illegal quant carrier. A composed observation view combining the initial run
and those measured replacements is **366 MATCH / 19 DIFFER / 417 NOT_RUN**,
OFF 772/PYC 386 rc0/R2=0, with R2 UNKNOWN 30/416. This is not a second full
802 execution at the repaired source. The full initial and affected-final
ledgers are separate in the [consumer report](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/latest_release_20261021.md),
which requires access to the private SSM repository.

Current first-stop groups retain 39 closed quant parameters, one overlong
Local binding stream, 371 SYS rejects, two row-index and two TCVT-shape rejects,
and one unmapped-Store UNKNOWN. Of the SYS group, actual executed-PC/ELF bytes
prove 330 literal STD plus canonical THREAD_ID; 41 other envelopes/addresses
remain UNKNOWN in this attribution. The 19 actual new-release DIFFER results
remain failures; sealed prior BFI evidence is context, not a freshly repeated
per-ELF trace claim. No unresolved pointer or model difference is reported as
an ELF defect. No new required public capability gap remains after this
bounded consumer repair; documented general format/rounding limits remain.
Old 476/439 (396 matched / 43 failed, original 37 separate) and every 799
ledger are unchanged. Remote action_required/SKIPPED is not a local-test pass.

The fixed-release producer-side observations and corrected GEMV attribution
are [cross-linked in Bench#235](https://github.com/PTO-ISA/SuperNPUBench/issues/235#issuecomment-6320715013). Only
proved released-asset violations are attributed there; PYC defects and
unresolved ownership remain separate.

## Public commit-state composition and compressed SYS consumer repair

`tracked_bit_transform.TrackedBitTransform` consumes the existing BitTransform
and records a coherent `WordCommitState` (last committed word, count, sticky
logical rejection) through one Rule. The public
[example and commands](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/README.md#composing-a-response-with-owned-state)
and [System bench](https://github.com/jiale-wangOwO/pyCircuit/blob/codex/functional-source-units/examples/functional_float/tracked_bit_bench.py) cover
inactive/invalid requests, hold and status effects, and both canonical BFI
endpoint shapes. Native workers1/2 and RTL pass8 cycles/16 epochs from the
same final IR. This is a reusable example model and explicit state policy;
compiler/runtime and the previously sealed numeric algorithms are unchanged.

The separate SSM repair is a missing accepted `C.BSTART.SYS` envelope0x0840,
not a public numeric API defect. Legal compressed/L32 SYS reads, actual
nonzero Stores and negative-domain preservation are independently checked
there. The [bounded consumer report](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/owner_repair_20261023.md)
requires SSM access and distinguishes the former41 non-SYS inputs, the
source/API construction behind Store0, one current-release BFI first
counterexample and the new feedback's wrapping word. Old476/799/802 results
are not overwritten. No candidate ASL PR398/399 or hosted engineering ABI is
silently treated as accepted, and no SDK/general numerical oracle is rerun.

The actual released-asset envelope/object-binding evidence is
[cross-linked in Bench#235](https://github.com/PTO-ISA/SuperNPUBench/issues/235#issuecomment-6368765894). The local compressed-SYS
fix is explicitly excluded from Bench defect attribution.
