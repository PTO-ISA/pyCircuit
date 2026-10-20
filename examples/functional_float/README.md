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
