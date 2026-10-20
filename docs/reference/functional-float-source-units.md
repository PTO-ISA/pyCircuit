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
| `float_compare.FloatCompare` | Binary32/binary64 eq/ne/lt/ge, explicit quiet/signaling policy; all comparisons false for NaN |
| `float_to_integer.FloatToInteger` | Binary32/binary64 to signed/unsigned 8/16/32/64-bit integers; RNE/RTM/RTP/RTZ/nearest-away, saturated invalid endpoints and NV/NX |
| `float_from_integer.FloatFromInteger` | Signed/unsigned 64-bit integers directly rounded once to binary32, binary64, binary16 or bfloat16 (format selector 0/1/2/3) |
| `fp32_max.FP32Max` | Binary32 maximumNumber/minimumNumber, signed-zero tie and signaling-NaN effects |
| `fp32_to_fp16.FP32ToFP16`, `fp32_to_bf16.FP32ToBF16` | Binary32 to binary16/bfloat16 conversion and effects |
| `input_expand.InputExpand` | Exact binary16/bfloat16 raw expansion to binary32 |
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
A canonical IEEE-invalid result is represented by `invalid=1` and a known NaN;
the arithmetic leaves clear `valid` for that outcome. An IEEE consumer can
commit the defined NaN and NV after checking the whole response. A response
with neither `valid` nor `invalid` authorizes no numerical commit. Request
availability, IEEE invalid, and an unknown runtime wire are distinct conditions.
Consumers must inspect the actual response contract rather than treating all
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
and quiet/signaling comparisons; 5,856 floating-to-integer raw-and-status vectors over both formats, five rounding modes and eight integer targets; and 4,112 bit-loop reference cases on native
and RTL. Native drivers compile with `-O1`. The example build driver also
compiles, links and emits every published top; emission alone is not numerical
execution coverage of every provider.

The exp polynomial is an approximation, not correctly-rounded transcendental
certification. Other rounding modes, exhaustive IEEE inputs, all format pairs,
fused operations, and a complete ISA are outside this example's claim. The
SSM release ledger (476 fixed inputs, including 439 historical PASS and 37
original FAIL) is consumer evidence and remains separate from these numerical
oracles. Its current OFF/PYC results and exceptions belong in the SSM report;
a historical 439/439 does not certify the current branch.
