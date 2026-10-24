# Functional numeric source units

This example keeps the IEEE-like floating-point algorithms in ordinary
source-captured Python and exposes them through the public `pycircuit` compiler
commands. The model bodies are reusable `@ac.module` definitions; they do not
depend on a private SSM runtime or a pre-generated C++ bundle.

The reusable top units are:

- `functional_float.fp32_product.FP32Product`
- `functional_float.fp32_sum.FP32Sum`
- `functional_float.fp32_product_sum.FP32ProductSum`
- `functional_float.fp32_exp.FP32Exp`
- `functional_float.fp32_divide.FP32Divide`
- `functional_float.word_arithmetic.WordArithmetic`

`FP32ProductSum` composes the independently published Product and Sum units and
performs separate multiply-round then add-round; it is not an FMA. Each result
carries `valid`, `invalid`, `overflow`, `underflow`, `inexact`, and `zero`.
The model uses fixed-width bits, explicit shifts, masks, integer division,
and selection. ProductSum retains invalid, overflow, underflow and inexact
from both stages; its zero flag describes the final Sum payload. IEEE invalid
does not suppress the Sum request.

`FP32Exp` uses range reduction by ln(2), a sixth-order Horner polynomial and
power-of-two scaling. Every multiply/add rounds independently to binary32.
It is an approximation, with the same algorithm as the earlier bit graph;
it is not a correctly-rounded transcendental certification. `FP32Divide`
normalizes significands, divides widened integers, retains the remainder as
sticky information and applies shared RNE packing. Reciprocal is composition
with a binary32 numerator of one. Special inputs use deterministic NaN,
infinity and signed-zero payloads without host floating-point evaluation.
The quotient response additionally exposes `divide_by_zero`, independently
from IEEE invalid and request rejection.

## Compile, link, and emit

Run from the pyCircuit checkout with its native compiler tools available:

```bash
ROOT=examples/functional_float
OUT=.pycircuit_out/functional_float
mkdir -p "$OUT/units" "$OUT/linked"

PYTHONPATH=python python3 -m pycircuit.cli compile \
  -c "$ROOT/fp32_types.py" --source-root "$ROOT" \
  --package-prefix functional_float -o "$OUT/units/types" --replace
PYTHONPATH=python python3 -m pycircuit.cli compile \
  -c "$ROOT/fp32_product.py" --source-root "$ROOT" \
  --package-prefix functional_float -I "$OUT/units/types" \
  -o "$OUT/units/product" --replace
PYTHONPATH=python python3 -m pycircuit.cli compile \
  -c "$ROOT/fp32_sum.py" --source-root "$ROOT" \
  --package-prefix functional_float -I "$OUT/units/types" \
  -o "$OUT/units/sum" --replace
PYTHONPATH=python python3 -m pycircuit.cli compile \
  -c "$ROOT/fp32_product_sum.py" --source-root "$ROOT" \
  --package-prefix functional_float \
  -I "$OUT/units/types" -I "$OUT/units/product" -I "$OUT/units/sum" \
  -o "$OUT/units/product_sum" --replace

PYTHONPATH=python python3 -m pycircuit.cli link \
  "$OUT/units/types" "$OUT/units/product" "$OUT/units/sum" \
  --top functional_float.fp32_product.FP32Product \
  -o "$OUT/linked/product.ac" --replace
PYTHONPATH=python python3 -m pycircuit.cli emit \
  --target cpp -o "$OUT/cpp_product" "$OUT/linked/product.ac" --replace
```

The emitted source directory contains the generated header/source and
`pycircuit_support.hpp`. A consumer can include those files and link the
Runtime component selected by the public pyCircuit install. The generated
module is a reusable value/effect boundary; ELF loading, memory commit, tile
state, syscalls, and scheduling remain consumer responsibilities.

## Scope and evidence

The existing independent integer oracle covers Product/Sum vectors, including
separate-rounding behavior and invalid/overflow effects. The source-unit test
shape is intentionally bounded; it is not a full IEEE-754 certification and
does not make the SSM 476/439 release report a certification of every format.
The model can be inspected and rebuilt from source using the commands above;
generated C++ and RTL are build outputs and should stay outside the source
commit unless a release archive policy explicitly requires them.

The additional providers use the same public commands and explicit closures:

```bash
for UNIT in exp divide; do
  PYTHONPATH=python python3 -m pycircuit.cli compile \
    -c "$ROOT/fp32_${UNIT}.py" --source-root "$ROOT" \
    --package-prefix functional_float \
    -I "$OUT/units/types" -I "$OUT/units/product" -I "$OUT/units/sum" \
    -o "$OUT/units/$UNIT" --replace
done
PYTHONPATH=python python3 -m pycircuit.cli link \
  "$OUT/units/types" "$OUT/units/product" "$OUT/units/sum" "$OUT/units/exp" \
  --top functional_float.fp32_exp.FP32Exp -o "$OUT/linked/exp.ac" --replace
PYTHONPATH=python python3 -m pycircuit.cli link \
  "$OUT/units/types" "$OUT/units/product" "$OUT/units/divide" \
  --top functional_float.fp32_divide.FP32Divide -o "$OUT/linked/divide.ac" --replace
for UNIT in exp divide; do
  for TARGET in cpp verilog; do
    PYTHONPATH=python python3 -m pycircuit.cli emit --target "$TARGET" \
      -o "$OUT/${TARGET}_$UNIT" "$OUT/linked/$UNIT.ac" --replace
  done
done
```

`tests/system/test_functional_float_source.py` covers the five public source
units, both emission targets, and native/RTL ProductSum exception propagation
with an input that distinguishes separate rounding from FMA. It uses the
configured CompilerDev tools and Runtime install; generated products remain
outside the checked-in example.

`WordArithmetic` is an independent source unit for wrapping 32/64-bit add,
subtract, multiply, logical operations and shifts, plus signed/unsigned division
and remainder. Its operation selector describes a numerical function rather
than an instruction encoding. Word results sign extend to 64 bits. Division by
zero yields zero; zero-denominator remainder yields the dividend. Unknown
selectors reject the request. Compile `word_arithmetic.py` without imports,
link the published unit with top `functional_float.word_arithmetic.WordArithmetic`,
and emit both targets through the same commands above.

## Additional public providers

The complete current inventory and value/effect contracts are maintained in the
[functional numeric reference](../../docs/reference/functional-float-source-units.md).
It includes binary64 Product/Sum/Divide using exact widened integer arithmetic,
quiet/signaling binary32/binary64 comparisons, direct signed/unsigned integer
conversion to four formats, min/max, binary16/bfloat16 casts and expansion,
row-major addresses, bounded integer elements, and reusable bit transformations.
They use the same public source-capture API; no ISA decoder or SSM state lives
in these providers.

Generate all providers, or just a selected top and its complete import closure:

```bash
PYTHONPATH=python python3 examples/functional_float/build_units.py \
  .pycircuit_out/functional_numeric --replace
PYTHONPATH=python python3 examples/functional_float/build_units.py \
  .pycircuit_out/functional_bits --only bit_transform --replace
```

`tests/system/test_functional_numeric_source.py` executes 5,852 independent
integer/rational raw-and-status vectors for binary64 arithmetic, four integer
conversion formats and quiet/signaling comparisons against both emitted targets.
It additionally checks 4,112 bit-loop reference cases against native and RTL.
Each separately linked C++ artifact is compiled in its own translation unit so
that its support header owns the correct nominal response declarations. A source
composition linked as one final design is the alternative when providers need
to be in the same generated hierarchy. Native test drivers use `-O1`; emission
or these bounded vectors do not establish exhaustive IEEE coverage.

`FloatToInteger` adds direct binary32/binary64 conversion to signed/unsigned
8/16/32/64-bit results. Its explicit rounding selectors are RNE, RTM, RTP,
RTZ and nearest-away (0..4); invalid conversion returns the corresponding
saturated endpoint with NV and without NX. The public numeric source test
checks 5,856 independent exact-rational raw-and-status vectors against both
native and RTL in addition to the earlier numerical and bit cases.

`FloatSqrt` uses 55 restoring integer-root steps, retaining guard/sticky bits
for the binary64 RNE result and for binary32. The shared `SqrtState` and digit
step are ordinary source providers. Zero sign, negative inputs, infinity,
quiet/signaling NaNs and NX are explicit. Native and RTL both match 1,060
independent integer-isqrt midpoint raw/status cases. This implementation uses
the existing compiler resource limits unchanged.

`WordDivision` adds an explicit zero-denominator policy while preserving the
existing WordArithmetic zero default. `FP32FloorModulo` provides the current
SSM-style divide/floor/product/subtract sequence, with every arithmetic step
rounded independently. Their public tests add 2,320 quotient-policy and 1,348
staged rational raw/status matches against both generated targets. See the
reference's consumer-policy section for the documented ASL differences and
special-value boundary; no FMA or host-float path supplies these results.


`FloatConvert` performs direct RNE conversion among binary32, binary64, FP16 and
BF16 (all 16 source/destination pairs), preserving signed zero and infinity and
reporting canonical NaN/IEEE effects. Its shared integer unpack/pack helpers
avoid an intermediate binary32 rounding. The public test adds 8,416 independent
raw/status cases on native and RTL, including subnormal ties and direct-rounding
traps. Build this top with `--only float_convert`; selectors and scope are in the
[functional numeric reference](../../docs/reference/functional-float-source-units.md).


`FloatFused` is a separate binary32/binary64 one-round multiply-add example for
explicit fused operations. It has 5,508 independent integer/rational native and
RTL raw/status matches. ProductSum remains separately rounded and does not
call this top. Use `--only float_fused` and consult the reference above for the
value/effect contract.


`FloatFusedFormat` adds direct one-round FP16/BF16 results while preserving the
existing two-format FloatFused interface; its independent native/RTL oracle has
11,016 raw/status matches over all four formats. `FP32Log` supplies a Q80,
25-term natural-log approximation with 1,043 Decimal100 native/RTL raw/status
matches. Both are selectable through `build_units.py`. Their algorithms,
exception behavior and bounded coverage are described in the reference above.


`FP19FiniteExpand` is an exact scalar expansion of a finite sign/E8/M10 scale
format to binary64, including exponent255 as a finite exponent. Native checks
cover all524,288 payloads;RTL checks cover5,632 directed/random payloads.
Select `--only fp19_expansion`; the numeric reference explains its distinct
finite-format contract.


`FP32NegativePower` constructs encoded negative binary32 powers with a declared
quiet-NaN sentinel. Every one of256 codes matches an exact-rational native/RTL
reference, including normal/subnormal and underflow boundaries. Select
`--only fp32_negative_power`; arithmetic effects remain with its consumers.


The reusable `lowp_decode.LowpDecode` and `lowp_encode.LowpEncode` examples
provide integer-only E4M3FN (kind0), E5M2 (kind1) and HiF8 (kind2) codecs.
Decode expands every raw byte exactly to binary32, with canonical special
values. Encode accepts binary32 and returns raw bytes plus effects. FP8 uses
RNE; HiF8 also supports RHB ties toward positive infinity and explicit finite
saturation. E4M3FN clamps overflow to its largest finite value without OF;
HiF8 reports tininess after rounding; a tiny inexact value that rounds to the minimum normal sets NX without UF. These example conventions are
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

`functional_float.fp32_trunc_modulo.FP32TruncModulo` is an additional exact
binary32 remainder policy with a quotient truncated toward zero. It uses
integer modular exponentiation over normalized significands, then directly
encodes the representable remainder; it does not round an intermediate
floating quotient. Exact finite cancellation produces +0, a zero dividend
retains its sign, and invalid operands produce canonical NaN plus invalid.
Build it with `build_units.py OUT --only fp32_trunc_modulo --replace`.
The focused `test_fp32_trunc_modulo_native_and_rtl` uses an independent
Fraction oracle: 1,350 native and 1,350 RTL raw/status matches, including
subnormals, the full exponent gap and quotient-rounding counterexamples.
`FP32FloorModulo` keeps its separately documented staged floor policy.


`float_to_integer_bits.FloatToIntegerBits` is a separate finite-overflow policy
from `FloatToInteger`. It accepts raw binary32/binary64, a dynamic width1..64,
signedness, RNE/RTM/RTP/RTZ/RNA/ROD/RHB (selectors0..6), and saturation selection.
Finite out-of-range results report OF|NX and clamp or wrap; NaN returns zero
with NV and infinity returns the selected endpoint with NV. Signed output is
sign-extended to64 bits. Its independent rational oracle checks3,392 native
and RTL result/status vectors. Build with `--only float_to_integer_bits`.
Architectural consumers must select their own special-value/offset policy;
this provider does not implicitly implement a particular ISA conversion.

`finite_lowp_cast.FiniteLowpCast` rounds raw binary64 directly once to E4M3,
E5M2 or HiF8 (kind0/1/2). Its explicit domain is finite input with magnitude
no greater than the selected maximum finite output. Other requests reject.
RNE is supported for all three formats, and HiF8 additionally supports RHB.
Exact Q96 distances retain binary64 information at HiF8 midpoints. The
independent rational/codebook oracle checks3,033 native and RTL vectors,
including adjacent binary64 values at ties. Build with
`--only finite_lowp_cast`; consumers handle out-of-domain and architectural
special-value policies explicitly. This avoids an intermediate binary32
rounding when composing an exact widened product with a final lowp cast.

The HiF8 encoder regression includes raw binary32 `0x37ffffff`, which rounds
to the minimum normal: result`0x7e`, NX=1, UF=0. The affected encode-only gate
has20,348 independent native/RTL raw/status matches. This generic codec's
post-round tininess policy is separate from an ISA operation that explicitly
selects a different conversion owner.


`float_round_integral.FloatRoundIntegral` composes the public dynamic-width
integer rounding and integer-to-float providers to round binary32/binary64 to
integer-valued results in the same format. Its seven policies0..6 match
`FloatToIntegerBits`, including RHB ties toward positive infinity. Large
already-integral values retain their encoding; NaNs canonicalize and signaling
NaNs report NV, infinity and signed zero are preserved. Native and RTL each
match4,340 independent rational/raw/status vectors. ISA consumers still apply
their own allowed policy subset: for example, a TCVT owner can reject explicit
RTO while the generic algorithm continues to expose odd rounding.

`packed4_encode.Packed4Encode` and `packed4_decode.Packed4Decode` provide
E2M1/E1M2 (kind0/1) codebooks. Encode accepts binary32, rounds once with RNE,
and exposes a three-bit overflow endpoint policy input. Decode expands all16
nibbles exactly to binary32, including both signed zeros. The endpoint is
explicit: choosing6 versus7 for E2M1 deliberately chooses different finite
values on overflow; consumers must follow their own owner rather than assuming
a library-wide saturation rule. Encode maps quiet NaN to zero, signals NV for
signaling NaN, and reports OF|NX for infinity/finite overflow. E2M1 tininess is
after rounding; E1M2's declared policy uses source tininess below1/4. The
independent rational codebook checks have2,448 encode and64 decode native/RTL
result/status matches. These are value codecs, not a packed Tile layout,
HiF4 conversion, complete FP4 arithmetic library, or ISA decoder.

## Composing a response with owned state

`tracked_bit_transform.py` adds `TrackedBitTransform`, a reusable stateful
consumer of the existing public `BitTransform` source unit. Its grouped
`WordCommitState` snapshot contains the last committed word, a wrapping u32
commit count, and a sticky logical rejection bit. One cohesive Rule updates the
three fields: an active valid request commits its computed result; an active
out-of-domain request sets rejection and holds value/count; an inactive request
holds all state. This is a word-provider policy, not the IEEE floating-invalid
response policy. Do not apply its `valid` guard blindly to a floating response
that defines a NaN and NV with `invalid=1`.

The example exposes no author clock/reset or primitive state pins. Work
observes committed state; the compiler and shared SystemRunner own sampling,
whole-system checks and Xfer. `tracked_bit_bench.py` has eight independent
expected snapshots, including ordinary and wrapping insertion, full-word
replacement, inactive invalid requests and sticky rejection. Expected words
include `0x200000002` for first32/last63 with inputs2/2 and
`0x8000000000000000` for first63/last32 with inputs1/1. This is an example
algorithm/state policy, not a new ISA opcode or a new compiler primitive.

Use the installed CompilerDev tools plus Runtime for source compile/link/emit.
Runtime-only users can consume generated C++ without LLVM; they do not compile
Python source units. The ordinary standalone public example flow is:

```bash
cmake -S examples/functional_float -B "$BUILD_WORD_STATE" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PYC_TOOLCHAIN_ROOT"
cmake --build "$BUILD_WORD_STATE" --parallel 4
ctest --test-dir "$BUILD_WORD_STATE" --no-tests=error --output-on-failure
```

It is registered in the existing nightly example catalog. The bounded test
passes native workers1/2 and Icarus RTL for8 cycles/16 sampling epochs, using
the same verified final IR (the standard verifier checks final-IR equality).
No SSM tree, ELF, private bundle or SDK rebuild is required. The source-only
`build_units.py --only tracked_bit_transform` entry separately publishes its
explicit provider closure to C++ and RTL. It does not execute Python designs
to supply DUT results. This test is scoped to the shown word/state behavior,
not a rerun of the general finite numerical oracles or all API/nightly gates.

The consumer's separate [latest802 first-difference audit](https://github.com/LinxISA/SuperScalarModel/blob/codex/functional-pyc-native-integration/functional_pyc/docs/remaining18_bfi_20261024.md)
checks actual release instruction fields and source availability, using an
independent ASL bit loop. Its18 newly observed first BFI results agree with PYC
on the `0x200000002` word above and differ from OFF. That private SSM report is
consumer evidence, not an additional public-example test or proof about later
program execution; no example/provider behavior or public gate changes here.
