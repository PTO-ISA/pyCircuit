# One-hot encoding pipeline

The original EncodedFlags13 passes through a depth-one, latency-one input queue,
a low-order one-hot rule and a depth-one, latency-one result queue. The rule
copies `flags: u8` and overwrites `index: u3`, `valid: u1`, and `conflict: u1`.
Their packed positions are flags 12:5, index 4:2, valid 1 and conflict 0.
Zero and multi-hot flags remain ordinary tokens: payload valid/conflict never
filter tokens or control queue protocol validity.

`OnehotEncode(valid, data, take)` returns Result15: input ready at bit 14,
output protocol valid at 13, and the complete record at 12:0. Exactly two
complete-token slots hold 26 logical payload bits. Ordinary typed fields and
queue connections describe the design; clocks, reset and storage proposals
remain compiler-owned.

Both queues explicitly use `downstream_pop`. A full replacement retires the
old result, transforms the old input and accepts a fresh input. Unstalled E0
accepts, E1 commits the result, and E2 first retires it. There is no empty bypass.
Held/falling clock levels transfer nothing. Rising reset drops occupied tokens
and empties both owners. Initialized invalid output is packed zero and must not
be confused with an encoded zero-flags token.

Boolean valid/conflict fields use explicit u1 storage under the
reviewed physical mapping. The unpacked helper flags stay logical Boolean until
their fresh field conversions; stored projections are fixed u1. This does not
claim logical Boolean nominal-field roundtrips.

For known flags, index selects the lowest asserted bit, or zero if empty. Valid
means at least one asserted bit; conflict means more than one. Four-state index
uses ordered pure conditional merging, valid uses OR reduction, and any input
X/Z makes conflict X. Computed fields contain no Z, and latent values under
computed X are unspecified. Copied flags retain every raw native plane. All
five previous derived bits are overwritten even if they contain X/Z. Historical
procedural encoder variants disagreed on unknowns; the current pure-fold policy
is explicit rather than claiming all-donor four-state parity.

## Build and verification

```sh
cmake -S examples/onehot_encode -B /absolute/build/onehot_encode -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/onehot_encode --parallel 4
ctest --test-dir /absolute/build/onehot_encode --output-on-failure --no-tests=error
```

Independent native and RTL oracles cover every byte with all 32 previous
index/flag combinations, old metadata 0/1/X/Z overwrite, source-bit uncertainty,
dense patterns, both native latent alternatives and known recovery. Independent
actual accept/retire/reset-drop ledgers check two-slot conservation, full
replacement, stalls, changed held offers, busy reset, drain and failure/discard/
Reset behavior. Native workers 1/2, known Verilator and genuine full-DUT Icarus
execute the generated design.

Standalone gates pass 2/2. Native workers 1/2 and Verilator agree on 16,425
known Work samples; native and genuine Icarus also agree on 557 four-state
samples. All 8,192 byte/prior-metadata combinations execute. The 248 four-state
offers cover per-bit, literal and dense patterns, all old metadata bits and
both native latent alternatives.

Known history accepts 8,199 tokens, retires 8,195 and reset-drops four;
four-state history is 260/256/four. Both finish empty at peak occupancy two.
Public-owner probes check discard/reprepare and failed proposals with exactly
one retained result; terminal executor failure preserves the epoch, invalidates
sampling and requires Reset before recovery. The finite runner limit is 20,000
sampling epochs per successful history.

## Scenario bench

`bench.py` stores 344 regular-clock scenarios in an immutable typed Table. The
16-bit phase counter holds at 343; an additional all-zero row preserves the
helper's result for every known phase from 344 through 65,535. The run retains
its 344-cycle limit. The complete advance and system bodies,
including assertions, logs and registration order, are unchanged.

An independent source check covers all 786,432 known-u16 field values,
including the zero fallback. Original and compact systems have identical complete
C++ workers 1/2 and RTL observations after removing only changed source-position
metadata. The original module drivers retain physical clocks, midstream reset,
failure and four-state histories; this does not claim arbitrary X/Z-phase
equivalence for the closed system helper. See the verified module
[excerpts and receipt](GENERATED.md).

The two events named `valid` report protocol and payload validity, in that order.

The source shrinks from 3,733 lines / 97,776 bytes to
428 lines / 67,979 bytes. Measurements below use the same
checkout-built compiler (with constant-plane materialization), LLVM 22 C++
toolchain and `-O0` on one machine. Compile/build/emission and first-run values
are single serial observations; warm medians use three alternating full-length
pairs. Every fresh process includes initialization. These local measurements
do not guarantee improvements on other platforms or optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 2.934 s | 1.858 s |
| Emit C++ | 3.495 s | 3.129 s |
| Build C++ | 1.869 s | 1.520 s |
| First measured full C++ run | 1.603 s | 0.562 s |
| Warm C++ median, one worker | 0.913 s | 0.063 s |
| Warm Verilog median | 0.049 s | 0.045 s |
| Warm native peak RSS median | 6.30 MiB | 4.28 MiB |

Final IR decreases from 17,659,993 to
14,248,133 bytes. Generated C++ totals
4,527,908 → 3,997,786 bytes; Verilog
totals 982,373 → 754,424 bytes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/onehot_encode --target cpp --cycles 344 --build-dir .pycircuit_out/onehot_encode/system-cpp
pycircuit run examples/onehot_encode --target verilog --cycles 344 --build-dir .pycircuit_out/onehot_encode/system-verilog
```
