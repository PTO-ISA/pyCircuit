# Functional floating-point source units

This example keeps the IEEE-like floating-point algorithms in ordinary
source-captured Python and exposes them through the public `pycircuit` compiler
commands. The model bodies are reusable `@ac.module` definitions; they do not
depend on a private SSM runtime or a pre-generated C++ bundle.

The three top units are:

- `functional_float.fp32_product.FP32Product`
- `functional_float.fp32_sum.FP32Sum`
- `functional_float.fp32_product_sum.FP32ProductSum`

`FP32ProductSum` composes the independently published Product and Sum units and
performs separate multiply-round then add-round; it is not an FMA. Each result
carries `valid`, `invalid`, `overflow`, `underflow`, `inexact`, and `zero`.
The model uses fixed-width bits, explicit shifts, masks, and selection only.

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
