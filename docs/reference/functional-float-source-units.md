# Reusable functional numeric source units

pyCircuit publishes reusable computation as typed, source-captured Python
functions and `@ac.module` providers. These examples use the public
`pycircuit compile` → explicit `link` closure → verified final IR → `emit`
commands. C++ and Verilog are generated from the same linked artifact. No
Python design function is executed to calculate DUT results.

The [functional numeric example](../../examples/functional_float/README.md)
contains inspectable algorithm providers and a reproducible
[`build_units.py`](../../examples/functional_float/build_units.py) driver.
They exercise existing compiler and runtime capabilities; they are example
algorithms, rather than new built-in floating primitives or a complete IEEE
library. Imports refer to published source-unit interfaces. Link receives the
complete body closure, including the nominal response types.

## Public provider inventory

Each top below is prefixed with `functional_float.`. The source directory is
[`examples/functional_float`](../../examples/functional_float).

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
[`test_functional_float_source.py`](../../tests/system/test_functional_float_source.py)
and [`test_functional_numeric_source.py`](../../tests/system/test_functional_numeric_source.py).
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

The current SSM consumer selects its existing floor sequence and all-ones
scalar zero quotient. The referenced PTO ASL instead declares a truncating
floating remainder and zero scalar quotient. These differences are explicitly
consumer policy and are not an ISA-conformance claim for this example library.
The user chose to preserve SSM floor behavior for the paired release experiment;
neither the ISA/ASL nor the ordinary OFF arithmetic is modified by the example.


## Direct floating format conversion

[`FloatConvert`](../../examples/functional_float/float_convert.py) accepts a raw
64-bit carrier and source/destination selectors: binary32=0, binary64=1,
binary16=2, bfloat16=3. All 16 pairs perform one RNE step from the original
significand, including subnormal inputs and post-rounding tininess. The
[`float_formats.py`](../../examples/functional_float/float_formats.py) helpers
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

[`FloatFused`](../../examples/functional_float/float_fused.py) computes one-round
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

[`FloatFusedFormat`](../../examples/functional_float/float_fused_format.py)
extends explicit one-round multiply-add to the same four format selectors as
FloatConvert. The raw carrier is 64 bits; binary16/bfloat16 results round
directly to their destination, rather than through binary32. Its exact-integer
reference checks 11,016 native/RTL raw-and-all-status cases, including all
earlier binary32/binary64 cases and 16-bit cancellation/subnormal inputs.
The existing `FloatFused(..., wide64, ...)` interface is preserved.

[`FP32Log`](../../examples/functional_float/fp32_log.py) normalizes the raw
binary32 significand, applies a 25-term atanh expansion with Q80 fixed-point
integers, and combines the exponent contribution using a fixed Q80 ln(2).
[`fp32_log_step.py`](../../examples/functional_float/fp32_log_step.py) holds
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

[`FP19FiniteExpand`](../../examples/functional_float/fp19_expansion.py) expands
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

[`FP32NegativePower`](../../examples/functional_float/fp32_negative_power.py)
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
HiF8 reports source tininess before rounding. These example conventions are
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
