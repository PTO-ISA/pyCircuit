# Bit primitive pipeline

The original Item28 passes through a depth-one, latency-one input queue, a
stateless measurement rule and a depth-one, latency-one result queue. The rule
preserves the byte's raw value/known/Z planes and overwrites every derived field.
It shares one low-order one-hot result for index, valid and conflict, alongside
one high-order priority result and the existing population/zero-count helpers.

| Item field | Packed bits | Meaning |
|---|---|---|
| `value: u8` | 27:20 | Original byte, copied unchanged |
| `priority_index: u3` | 19:17 | Lowest asserted bit, zero for empty input |
| `high_index: u3` | 16:14 | Highest asserted bit, zero for empty input |
| `priority_valid: u1` | 13 | At least one asserted bit |
| `onehot_conflict: u1` | 12 | More than one asserted bit |
| `population: u4` | 11:8 | Number of asserted bits |
| `leading: u4` | 7:4 | Leading zeros, eight for zero |
| `trailing: u4` | 3:0 | Trailing zeros, eight for zero |

`BitPrimitivePipeline(valid, data, take)` returns Result30: input ready at bit
29, output protocol valid at 28, and the complete Item in 27:0. There are exactly
two complete-token slots, holding 56 logical payload bits. Python describes
ordinary typed fields, a rule and queue connections; clock/reset and storage
proposal/commit details stay compiler-owned.

Both queues explicitly use `downstream_pop`. A full replacement retires the old
result, transforms the old input and accepts a new input. Unstalled E0 accepts,
E1 commits the transformed result, and E2 first retires it. There is no empty
bypass. Held and falling clock levels transfer nothing. Rising reset drops both
occupied slots. Initialized invalid output is packed zero; it is not a measured
zero-byte token. Previous derived values and unknowns are always overwritten.

The Boolean `onehot_conflict` field uses explicit u1 storage. The
helper's flags remain logical Boolean before their fresh field conversion;
stored projections are fixed u1. This physical mapping does not establish
logical Boolean nominal-field transport.

## Build and verification

```sh
cmake -S examples/bit_primitive_pipeline -B /absolute/build/bit_primitive_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/bit_primitive_pipeline --parallel 4
ctest --test-dir /absolute/build/bit_primitive_pipeline --output-on-failure --no-tests=error
```

Independent native/RTL tests cover the full byte domain, prior field ranges,
all old derived bits' 0/1/X/Z overwrite, per-bit source uncertainty, both native
latent alternatives and known recovery. A separate old-slot scoreboard checks
real accept/retire/reset-drop conservation, full replacement, stalls, held
offers, busy reset, drain and failure/discard/Reset behavior. Native workers
1/2, known Verilator and genuine full-DUT Icarus execute generated models.

Standalone gates pass 2/2. Native workers 1/2 and Verilator agree on 42,153
known Work samples; native and genuine Icarus also agree on 813 four-state
samples. The known corpus contains all 256 byte values, 320 prior index
combinations and 20,480 prior count combinations. The 376 four-state offers
include per-bit and endpoint uncertainty, explicit patterns, all old metadata
bits and both latent alternatives.

Known history accepts 21,063 tokens, retires 21,059 and reset-drops four;
four-state history is 388/384/four. Both drain empty and reach occupancy two.
Three direct-owner probes verify explicit discard, failed acceptance and failed
retirement without partial commit. Isolated native terminal cases require Reset
before recovery; isolated RTL negative cases diagnose uncertain effective
transfers. The finite runner bound is 45,000 sampling epochs per history.
The separate `bench.py` system checks a finite regular-clock known-state scenario with exact old-state output checks. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/bit_primitive_pipeline --target cpp --cycles 344 --build-dir .pycircuit_out/bit_primitive_pipeline/system-cpp
pycircuit run examples/bit_primitive_pipeline --target verilog --cycles 344 --build-dir .pycircuit_out/bit_primitive_pipeline/system-verilog
```
