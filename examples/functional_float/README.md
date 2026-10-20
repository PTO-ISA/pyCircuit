# Functional floating-point source units

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
