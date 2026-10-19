# Functional floating-point source units

The supported current-main way to publish a reusable numeric block is a
source-captured `@ac.module` source unit. The `functional_float` example keeps
its Product, Sum, and separately-rounded ProductSum algorithms in Python;
`pycircuit compile`, `link`, and `emit` are the only compiler steps.

The example lives at
[`examples/functional_float`](../../examples/functional_float/README.md). Its
source bodies are ordinary fixed-width bit graphs. They do not call host
floating point, a legacy calculator, or a private SSM adapter.

## Units

| Source | Public top | Contract |
| --- | --- | --- |
| `fp32_product.py` | `functional_float.fp32_product.FP32Product` | binary32 multiply, RNE, explicit validity and IEEE-like effects |
| `fp32_sum.py` | `functional_float.fp32_sum.FP32Sum` | binary32 add, RNE, explicit validity and IEEE-like effects |
| `fp32_product_sum.py` | `functional_float.fp32_product_sum.FP32ProductSum` | multiply-round then add-round; not FMA |

The generated response fields are `result`, `valid`, `invalid`, `overflow`,
`underflow`, `inexact`, and `zero`. `valid` is the protocol/defined-result
gate; a consumer must not commit a destination value when it is false. These
are bounded design-neutral graphs, not a claim of exhaustive IEEE-754 or all
format coverage.

## Public command flow

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
PYTHONPATH=python python3 -m pycircuit.cli link \
  "$OUT/units/types" "$OUT/units/product" "$OUT/units/sum" \
  --top functional_float.fp32_product.FP32Product \
  -o "$OUT/linked/product.ac" --replace
PYTHONPATH=python python3 -m pycircuit.cli emit \
  --target cpp -o "$OUT/cpp_product" "$OUT/linked/product.ac" --replace
```

The emitted C++ is a reusable value/effect module. The host supplies inputs,
applies the generated value and status outputs, and owns memory ordering,
architectural state, tile publication, ELF loading, and scheduling. Generated
C++, RTL, `.pyc`, and build directories stay outside the source tree.

## Version boundary

This page describes the current source-capture API on pyCircuit `main`. It does
not document the removed `Circuit`/`build_functional`/`floating` authoring
interface. The local `8a728450` functional lane and its frozen C++ bundle are
not a compatibility guarantee for this source-unit API. The SSM-owned
`412a180` testbed is a separate consumer lane and its five representative
fixp ELFs are not a certification of this example or of the full release set.

The fixed SSM release result (476 rows, 439 baseline-PASS targets, 37
baseline-original failures) is consumer evidence. It must not be presented as
a general numerical certificate for every format, rounding mode, NaN payload,
or generated source unit.
