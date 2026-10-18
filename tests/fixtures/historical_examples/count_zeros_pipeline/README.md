# Leading and trailing zero-count pipeline

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named ingress and
egress queues retain the two depth-one, latency-one `downstream_pop` owners.
One registered `transfer` Rule pops the old ingress head, computes the payload
and pushes the egress atomically; the compiler supplies availability and capacity
guards. Raw `in_valid`/`in_data` and `out_ready` bindings retain the external
handshake, and `ready`/`valid`/`data` expose raw queue boundary views. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

`Item` contains `value: u13`, `leading: u4` and `trailing: u4`. A depth-one,
latency-one source queue feeds a transform and one depth-one, latency-one result
queue. The transform preserves value and overwrites both old count fields using
the generic fixed-bit helpers. Both queues explicitly use `downstream_pop`:
full replacement consumes the old head and retains occupancy, with no empty
bypass. There are exactly two complete-token slots, holding 42 logical payload
bits, and no additional state.

`CountZerosPipeline(valid, data, take)` returns `CountZerosResult`: ready at bit
22, valid at 21 and Item at 20:0. Item packs value at 20:8, leading at 7:4 and
trailing at 3:0. Python exposes ordinary typed fields and queue connections;
clock/reset and Work/Xfer storage details remain compiler-owned.

An unstalled token is accepted at E0, transformed into the result queue at E1
and first consumed at E2. Work observes old heads. Held and falling clock levels
do not transfer. Rising reset empties both queues; initialized invalid output
is the all-zero packed Item, without evaluating it as a counted zero token.

Known zero returns 13 for both counts; all ones returns zero. A one-hot bit b
returns leading `12-b` and trailing b. X/Z follows the historical RTL count-tree
contract: unknown suffixes after a known one are ignored; endpoint-only
uncertainty can retain known-zero upper count bits; other prefix uncertainty
makes the natural count all-X. Counts contain no Z, and latent computed value
bits under count-X are unspecified. The preserved value13 field must retain
every native value/known/Z bit. Old count fields, including 14, 15, X and Z, are
overwritten and must not affect the new result.

## Build and verification

```sh
cmake -S examples/count_zeros_pipeline -B /absolute/build/count_zeros_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/count_zeros_pipeline --parallel 4
ctest --test-dir /absolute/build/count_zeros_pipeline --output-on-failure --no-tests=error
```

Independent tests must exercise the full value13 domain, varied old count fields,
opposite-direction results, two-slot conservation, replacement, stalls, reset,
holds and finite drain. Native workers 1/2, Verilator and genuine Icarus run the
generated DUT; native comparisons distinguish strict copied planes from computed
count-X masks. Failed systems require Reset before reuse.

The standalone module, system and four-state gates pass 3/3. Native workers 1/2,
Verilator and Icarus agree on 18,985 known Work samples; native and genuine
Icarus agree on 717 four-state samples. The known corpus contains all 8,192
value13 values plus 1,280 combinations of old count fields. The 328 four-state
vectors cover every value bit, endpoint distances, exact partial-known patterns,
unknown old counts and dense patterns, including native latent alternatives.

The known ledger accepts 9,479 tokens, retires 9,475 and reset-drops four;
the four-state ledger is 340/336/four. Both finish empty at peak occupancy two.
The driver checks E0/E1/E2, full replacement, holds, reset and public-owner
discard/reprepare. A separate terminal unknown-handshake case requires Reset
before reuse. Copied value bits retain all planes; count-X latent bits remain
unasserted. The finite runner limit is 20,000 ticks per successful history.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
10-field, 46-bit `Stimulus` preserves the original schema, field order and all
input/expected payloads. Row 184 is all zero, retaining the helper fallback
for every known u16 phase from 184 through 65,535. This fallback is distinct
from the system counter, which holds at 183. The run retains 184 cycles.

The 13-bit value and both original four-bit count inputs remain explicit,
including old values 14 and 15 that the DUT overwrites. Leading and trailing
expectations stay independent. Invalid empty output still expects packed zero;
it is not recomputed as a valid counted-zero token with two counts of 13.

The complete advance and system bodies are unchanged, including unconditional
assertions, observation order and advance-before-check/exercise registration.
Independent source checks compare all 655,360 field values across the known-u16
helper domain. Twenty complete native and RTL traces match all 2,208 observations
and the terminal result after excluding only changed source-position metadata,
including native workers 1 and 2. Baseline and compact versions each pass all
three standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical tests retain 18,985 known and 717 four-state Work samples,
with the reset, held-clock and conservation checks described above. The
registered four-state test already executes Icarus. Regular-clock systems do not
replace physical histories, and known-u16 source proof does not establish
arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,756 lines / 46,015 bytes to
267 lines / 32,500 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.285 s | 0.917 s |
| Link system | 2.084 s | 1.686 s |
| Emit C++ | 1.810 s | 1.586 s |
| Emit Verilog | 1.508 s | 1.255 s |
| Build C++ | 1.261 s | 1.075 s |
| Build Verilog | 1.568 s | 1.513 s |
| First measured full C++ run | 0.362 s | 0.137 s |
| First measured full Verilog run | 0.279 s | 0.122 s |
| Warm C++ median, one worker | 0.251391 s | 0.033921 s |
| Warm Verilog median | 0.023858 s | 0.023263 s |
| Warm native peak RSS median | 3.89 MiB | 2.94 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 8,455,579 bytes | 6,725,150 bytes |
| Generated C++ total | 2,176,020 bytes | 1,911,222 bytes |
| Generated Verilog total | 469,891 bytes | 369,105 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
For the recorded comparison, DUTs, drivers, configuration, cycle limits and
timeouts were unchanged.

```sh
pycircuit run examples/count_zeros_pipeline --target cpp --cycles 184
pycircuit run examples/count_zeros_pipeline --target verilog --cycles 184
```
