# BF16 Fused Multiply-Accumulate (FMAC)

A BF16 floating-point fused multiply-accumulate unit with 4-stage pipeline,
built from primitive standard cells (half adders, full adders, MUXes).

## Operation

```text
acc_out (FP32) = acc_in (FP32) + a (BF16) × b (BF16)
```

## Formats

| Format | Bits | Layout | Bias |
|--------|------|--------|------|
| BF16 | 16 | sign(1) \| exp(8) \| mantissa(7) | 127 |
| FP32 | 32 | sign(1) \| exp(8) \| mantissa(23) | 127 |

## 4-Stage Pipeline — Critical Path Summary

```text
  Stage 1: Unpack + PP + 2×CSA    depth = 13  ██████
  Stage 2: Complete Multiply       depth = 22  ███████████
  Stage 3: Align + Add            depth = 21  ██████████
  Stage 4: Normalize + Pack       depth = 31  ███████████████
  ──────────────────────────────────────────────
  Total combinational depth       depth = 87
  Max stage (critical path)       depth = 31
```

| Stage | Function | Depth | Key Components |
|-------|----------|------:|----------------|
| 1 | Unpack BF16, exp add, **PP generation + 2 CSA rounds** | 13 | Bit extract, MUX, 10-bit RCA, AND array, 2× 3:2 CSA |
| 2 | Complete multiply (remaining CSA + carry-select final add) | 22 | 3:2 CSA rounds, 16-bit carry-select adder |
| 3 | Align exponents, add/sub mantissas | 21 | Exponent compare, 5-level barrel shift, 26-bit RCA, magnitude compare |
| 4 | Normalize, pack FP32 | 31 | 26-bit LZC (priority MUX), 5-level barrel shift left/right, exponent adjust |

**Pipeline balance**: The 8×8 multiplier is split across Stages 1 and 2.
Stage 1 generates partial products (AND gate array) and runs 2 rounds of
3:2 carry-save compression, reducing 8 rows to ~4.  The intermediate
carry-save rows are stored in pipeline registers.  Stage 2 completes the
reduction and uses a carry-select adder for the final addition.  This
achieves good balance: **13 / 22 / 21 / 31** (critical path in Stage 4).

## Design Hierarchy

```text
bf16_fmac.py (top level)
└── primitive_standard_cells.py
    ├── half_adder, full_adder        (1-bit)
    ├── ripple_carry_adder            (N-bit)
    ├── partial_product_array         (AND gate array)
    ├── compress_3to2 (CSA)           (carry-save adder)
    ├── reduce_partial_products       (Wallace tree)
    ├── unsigned_multiplier           (N×M multiply)
    ├── barrel_shift_right/left       (MUX layers)
    └── leading_zero_count            (priority encoder)
```

## Files

| File | Description |
|------|-------------|
| `primitive_standard_cells.py` | HA, FA, RCA, CSA, multiplier, shifters, LZC |
| `bf16_fmac.py` | 4-stage pipelined FMAC |
| `fmac_capi.cpp` | C API wrapper |
| `tests/integration/pycircuit/applications/test_fmac.py` | 100 runtime cases |

## Build & Run

```bash
# 1. Compile RTL
PYTHONPATH=python:. python -m pycircuit.cli emit \
    examples/pycircuit/applications/bf16_fmac/bf16_fmac.py \
    -o .pycircuit_out/applications/bf16_fmac/generated/bf16_fmac.pyc
$PYC_TOOLCHAIN_ROOT/bin/pycc .pycircuit_out/applications/bf16_fmac/generated/bf16_fmac.pyc \
    --emit=cpp --cpp-split=module \
    --out-dir=.pycircuit_out/applications/bf16_fmac/generated

# 2. Build shared library
c++ -std=c++20 -O2 -shared -fPIC \
    -I "$PYC_TOOLCHAIN_ROOT/include" \
    -I .pycircuit_out/applications/bf16_fmac/generated \
    .pycircuit_out/applications/bf16_fmac/generated/bf16_fmac.cpp \
    examples/pycircuit/applications/bf16_fmac/fmac_capi.cpp \
    "$PYC_TOOLCHAIN_ROOT/lib/libpyc6_runtime.a" \
    -o .pycircuit_out/applications/bf16_fmac/libfmac_sim.dylib

# 3. Run 100 test cases
python tests/integration/pycircuit/applications/test_fmac.py
```

## Test Results

100 test cases verified against Python float reference via true RTL simulation:

- **100/100 passed**
- **Max relative error**: 5.36e-04 (limited by BF16's 7-bit mantissa)
- **Test groups**: simple values, powers of 2, small fractions, accumulation
  chains, sign cancellation (acc ≈ -a×b), and 40 random cases
