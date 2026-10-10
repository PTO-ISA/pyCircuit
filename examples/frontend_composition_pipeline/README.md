# Frontend composition pipeline

## Data and transformation

| Item34 field | Packed bits | Result behavior |
|---|---|---|
| Header: opcode_bits4, tag4 | 33:26 | Preserve both fields |
| Patch: tag4, valid1 | 25:21 | Preserve both fields |
| Opcode4 | 20:17 | Preserve raw carrier |
| flags4 | 16:13 | Preserve flags |
| result_tag4 | 12:9 | Copy Patch.tag |
| result_valid1 | 8 | Copy Patch.valid |
| result_opcode4 | 7:4 | Write Opcode.WRITE, code9 |
| onehot_index2 | 3:2 | Lowest asserted flags bit, zero if empty |
| onehot_valid1 | 1 | OR of flags |
| onehot_conflict1 | 0 | More than one asserted flags bit |

Opcode is nominal and keeps NONE=0, READ=3, WRITE=9. Invalid codes and X/Z are
ordinary payload carriers. Zero or multi-hot flags remain tokens; payload valid
does not control protocol validity. The rule first creates a Packet from Header
using the default zero valid field, copies it and updates tag/valid from Patch,
then updates a copy of Item. It shares one pure onehot call's three results.
All thirteen previous derived bits are overwritten, including invalid old Enum
codes or X/Z. Header fields remain stored even when the computed results do not
use them.

Boolean fields use explicit u1 physical storage. The helper produces
logical Boolean flags and converts them at the existing fresh field boundary;
this does not claim logical Boolean nominal-field roundtrips. Record spread is
expressed as named construction and ordinary copy/field assignments. No compiler
special case, new primitive or backend path is involved.

The original prefix at bits33:13 and copied result tag/valid at12:8 preserve all
native value/known/Z planes, including latent bits. Result opcode is known9.
The low four computed onehot bits follow ordered pure conditional merging,
OR-valid and popcount>1 conflict. Their known/Z masks are exact; latent values
under computed X are unspecified and computed outputs contain no Z. Historical
procedural encoder variants disagreed on X/Z; this uses the reviewed current
helper policy rather than claiming equivalence to every retired variant.

## Queue timing

`FrontendCompositionPipeline(valid, data, take)` returns Result36: ready35,
protocol valid34 and Item34 below. Exactly two complete-Item queues provide
two token slots, 68 logical payload bits. Both have depth1, latency1 and explicit
`downstream_pop`. An unstalled token is accepted at E0, transformed into the
result queue at E1, and first retired at E2. Full replacement retires the old
result, transforms the old input and accepts a fresh input, with no empty bypass.

Held/falling clocks transfer nothing. Rising reset drops occupied tokens and
empties both owners. Initialized invalid output is packed zero, not a valid
transformed zero token. Python exposes ordinary types, rule and queue connections.
The compiler adds hidden physical clock/reset pins driven by the host/SystemRunner;
the shared Runtime manages Work/Xfer and the compiler-generated storage proposals.

## Build and verification

```sh
cmake -S examples/frontend_composition_pipeline -B /absolute/build/frontend_composition_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/frontend_composition_pipeline --parallel 4
ctest --test-dir /absolute/build/frontend_composition_pipeline --output-on-failure --no-tests=error
```

Independent tests cover all flags/raw-enum/patch-field combinations, old derived
field ranges, all header values, per-bit X/Z transport/overwrite and both native
latent alternatives. Separate old-slot and actual-DUT ledgers check two-slot
conservation, full replacement, stalls, changed held offers, busy reset, finite
drain, failed transfers without partial commit, discard and Reset recovery.
Native workers1/2, known Verilator and genuine full-DUT Icarus execute the same
generated design. The standalone module, system and four-state gates pass 3/3.
There are 18,281 known Work samples and 1,253 four-state samples. The 9,120 known
offers include 8,192 complete control combinations with every old thirteen-bit
metadata value, 672 explicit old-field range cases and all 256 header values.
The 596 four-state offers cover copied bits, old metadata overwrite, directed
one-hot patterns and dense unknowns, including both native latent alternatives.

Known history accepts 9,127 tokens, retires 9,123 and reset-drops four;
four-state history is 608/604/four. Both finish empty at peak occupancy two.
Three owner probes check explicit discard, unknown acceptance and unknown
retirement without partial commit. Separate terminal processes check failure,
unavailable sampling and Reset recovery. The finite runner bound is 20,000
sampling epochs per successful history.

## Scenario bench

`bench.py` stores 184 regular-clock scenarios in an immutable typed Table. The
16-bit phase counter holds at 183; an additional all-zero row preserves the
helper's result for every known phase from 184 through 65,535. The run retains
its 184-cycle limit. The complete advance and system bodies,
including assertions, logs and registration order, are unchanged.

An independent source check covers all 1,835,008 known-u16 field values,
including the zero fallback. Original and compact systems have identical complete
C++ workers 1/2 and RTL observations after removing only changed source-position
metadata. The original module drivers retain physical clocks, midstream reset,
failure and four-state histories; this does not claim arbitrary X/Z-phase
equivalence for the closed system helper. See the verified module
[excerpts and receipt](GENERATED.md).

The source shrinks from 4,093 lines / 122,818 bytes to
344 lines / 87,729 bytes. Measurements below use the same
checkout-built compiler (with constant-plane materialization), LLVM 22 C++
toolchain and `-O0` on one machine. Compile/build/emission and first-run values
are single serial observations; warm medians use three alternating full-length
pairs. Every fresh process includes initialization. These local measurements
do not guarantee improvements on other platforms or optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 3.356 s | 2.278 s |
| Emit C++ | 4.356 s | 4.375 s |
| Build C++ | 2.077 s | 1.668 s |
| First measured full C++ run | 1.102 s | 0.993 s |
| Warm C++ median, one worker | 0.555 s | 0.062 s |
| Warm Verilog median | 0.054 s | 0.052 s |
| Warm native peak RSS median | 6.81 MiB | 4.67 MiB |

Final IR decreases from 20,575,146 to
17,645,346 bytes. Generated C++ totals
5,026,898 → 4,919,622 bytes; Verilog
totals 1,103,383 → 890,104 bytes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/frontend_composition_pipeline --target cpp --cycles 184 --build-dir .pycircuit_out/frontend_composition_pipeline/system-cpp
pycircuit run examples/frontend_composition_pipeline --target verilog --cycles 184 --build-dir .pycircuit_out/frontend_composition_pipeline/system-verilog
```
