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

Prior standalone gates passed 2/2. Native workers 1/2 and Verilator agree on 42,153
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
The separate `bench.py` system checks the complete 344-cycle regular-clock
known-state scenario with exact old-state output checks. Its original `Stimulus`
retains 20 ordered fields and 60 bits. A same-source `Stimuli` Table stores 345
rows: the 344 original literal records followed by an all-zero `Stimulus()`.
Zero-valued fields are omitted using existing recursive-zero initialization.
The extra row is fallback data and adds no simulation cycle.

`stimulus(phase: bits[16])` selects its corresponding row below 344 and the zero
row for known phases 344–65535. The phase starts at zero, and the original
advance rule increments only below 343. It otherwise holds, including injected
high phases; it does not replace high values with 343. The complete advance and
system bodies retain their original AST: eight explicit Item fields, the DUT
call, 10 unguarded assertions, 11 unconditional logs, and `advance(phase)` before
`check()`.

The author derived literal rows from the frozen original AST without executing
or importing design code. Independent preservation of 6,880 active field values,
all known-u16 fallback values and the high-phase hold rule remains separate
from that extraction. The original independent drivers retain their full
physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/bit_primitive_pipeline --target cpp --cycles 344 --build-dir .pycircuit_out/bit_primitive_pipeline/system-cpp
pycircuit run examples/bit_primitive_pipeline --target verilog --cycles 344 --build-dir .pycircuit_out/bit_primitive_pipeline/system-verilog
```

## Verification and measured costs

The current module, system and four-state tests pass 3/3. Native workers 1/2 and Verilator retain 42,153 known Work samples; native and genuine Icarus retain 813 four-state samples. The system completes 344 cycles / 688 epochs, ten source-check definitions and 7,568 observations.

An independent source check preserves all 6,880 active field values and the complete 65,536-phase u16 function domain (1,310,720 values), including the all-zero fallback and hold-above-343 advancement. Ten mutated candidates are rejected.

Complete original and candidate observations agree across native workers 1/2 and RTL. The physical-control, reset/discard, token-ledger and four-state oracles remain separate from this known-phase source proof; arbitrary injected X/Z phase equivalence is not claimed. See the current [verified module excerpts and receipt](GENERATED.md).

The bench shrinks from 6,003 lines / 181,662 bytes to 472 lines / 131,214 bytes. Measurements use the same machine, installed compiler and LLVM 22 C++ toolchain with `-O0`. Build/emission entries are single observations from serial baseline and candidate pipelines; runtime entries are medians of three additional paired warm runs in alternating order, using the complete registered cycle count. These are example-specific observations, not a cross-platform guarantee.

| Phase | Original | Scenario table |
| --- | ---: | ---: |
| Compile bench | 5.58 s | 3.25 s |
| Link system | 6.44 s | 5.56 s |
| Emit C++ | 6.02 s | 5.59 s |
| Build C++ simulator | 3.01 s | 2.83 s |
| Emit Verilog | 4.73 s | 3.96 s |
| Build Verilog simulator | 2.67 s | 1.82 s |
| Run C++, one worker (warm median) | 3.200 s | 3.185 s |
| Run Verilog (warm median) | 0.160 s | 0.145 s |

Native runtime is approximately unchanged across these limited warm samples; one candidate run took 4.462 seconds versus 3.072 and 3.185 seconds in the other two runs. The table reports the median and does not claim a stable native speedup.

Final IR falls from 30,002,488 to 24,416,914 bytes. Generated C++ files total 7,490,737 → 6,821,142 bytes; generated Verilog files total 1,628,147 → 1,233,846 bytes.

Complete source-import and transformed artifacts are retained locally and reproduce both published source units byte-for-byte. Public execution and measured builds share identical final IR and generated outputs. No compiler API, DUT storage, scenario count, cycle limit or timeout changes are required.
