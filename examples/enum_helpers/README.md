# Sparse enum helpers

Opcode4 retains NONE=1, READ=3, WRITE=9 and ERROR=15. Command10 packs raw_opcode4
at9:6, onehot_mask2 at5:4 and nominal selector4 at3:0. All1,024 known combinations
are valid input tokens, including raw/selector codes without an enum member.

| EnumResult20 field | Packed bits | Behavior |
|---|---|---|
| decoded: Opcode | 19:16 | Checked member selection, NONE fallback |
| decoded_valid: u1 | 15 | Raw code belongs to the declared members |
| onehot: Opcode | 14:11 | READ/WRITE for low/high mask bit, NONE empty, ERROR conflict |
| onehot_present: u1 | 10 | OR of mask bits |
| onehot_conflict: u1 | 9 | popcount(mask)>1 |
| selected: u1 | 8 | Selector is READ or WRITE |
| classification: u8 | 7:0 | NONE/READ/WRITE/ERROR→10/11/12/13, invalid→255 |

Boolean fields use explicit u1 physical storage. Predicates remain
logical Boolean until the existing fresh field conversion; this does not claim
logical Boolean nominal-field roundtrips. All Enum fields are explicitly supplied
because Opcode has no zero member.

## Four-state meaning

The checked decoder and classification preserve the original balanced conditional
trees. Selecting the raw carrier when valid, or replacing the balanced tree with
a linear if/elif chain, can change partial-known bits. Onehot conflict must use
popcount, not bit-AND. The source keeps these circuits explicit and shares their
ordinary local values.

| Input | Required computed result |
|---|---|
| raw_opcode=X001 | decoded_valid=X, decoded=XXX1 |
| selector=00X1 | classification=XXXX1XXX |
| mask=0X | present=X, conflict=X, onehot=XXX1 |
| mask=X1 | present=1, conflict=X, onehot=XX11 |

Replacing X with Z produces the same computed symbols. All output fields derive
from comparisons, arithmetic or selection of known constants: computed output
contains no Z. Known/Z masks and known bits are contractual; latent values under
computed X are not. Invalid/unknown enum payloads do not fail or filter tokens.
Payload valid/present/conflict are separate from queue protocol valid.

## Queue timing

`EnumHelpers(valid, data, take)` returns Result22: input ready21, output valid20
and EnumResult20. The input Command10 and output EnumResult20 each have one
depth1/latency1 queue, for two token slots and30 logical payload storage bits.
Explicit downstream_pop permits old-head retirement and replacement on the same
edge. E0 accepts Command, E1 commits its result and E2 first retires it. There is
no empty bypass or extra queue for the returned value.

Held/falling clocks transfer nothing. Rising reset drops both occupied tokens.
Initialized invalid output remains physical packed zero, not a constructed result
with NONE members. The compiler generates hidden clock/reset pins driven by the
host/SystemRunner, and the shared Runtime manages Work/Xfer and discard.

## Build and verification

```sh
cmake -S examples/enum_helpers -B /absolute/build/enum_helpers -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/enum_helpers --parallel 4
ctest --test-dir /absolute/build/enum_helpers --output-on-failure --no-tests=error
```

Independent tests cover the full known Command domain, bounded exhaustive
four-state raw/selector fields and masks, literal distinctions above, dense cases
and both native latent alternatives. Separate 10-bit input and20-bit output
queue oracles check old-state timing, capacity two, replacement, stalls, holds,
busy reset, finite drain, conservation, discard and terminal Reset recovery.
The standalone module, system and four-state gates pass 3/3. Native workers1/2 and
Verilator agree on 2,089 known Work rows; native and genuine Icarus also agree
on 3,229 four-state rows. The corpus contains all1,024 known commands and1,584
four-state tokens:512 raw-field,512 selector-field,32 symbolic-mask,512 dense
crossed and16 literal-witness cases, including both native latent alternatives.

Known history accepts1,031 tokens, retires1,027 and reset-drops four;
four-state history is1596/1592/four. Both finish empty at peak occupancy two.
Three direct-owner probes check discard, failed acceptance and failed retirement
without partial commit. The discarded host-Reset probe must actually retire its
retained command's classified result exactly once before a real Reset. Separate
terminal native/RTL probes check unknown-control failure and native Reset recovery.
The finite runner bound is4,000 sampling epochs per successful history.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
14-field, 34-bit `Stimulus` retains the original schema and field order. Row 184
is all zero, preserving the helper fallback for every known u16 phase from 184
through 65,535. This fallback is distinct from the system counter, which holds
at phase 183. The run retains its original 184-cycle limit.

The Table contains the original Bits fields, so zero-filled rows do not construct
nominal Enums. The system still maps selector 1/3/9 to NONE/READ/WRITE and every
other value to ERROR, including selector zero in the helper fallback. Raw opcode
codes without members remain valid input scenarios. Both decoded and onehot
checks/logs retain `enum_to_bits`; invalid output expectations stay raw zero.

The complete system retains all 9 unconditional assertions, 10 observations,
ordered DUT inputs and `advance(phase)` before `check()`. Independent source
checks compare all 917,504 field values across the complete known-u16 helper
domain. Twenty complete native and RTL traces match all 3,680 observations
and the terminal result after excluding only changed source-position metadata;
native workers 1 and 2 are included. Baseline and compact versions each pass
all three standalone gates. See the module [excerpts and receipt](GENERATED.md).

The closed system covers a regular-clock known-state trajectory. Original
physical reset, held-clock and four-state histories remain with the unchanged
independent module drivers described above. Known-u16 source equivalence does
not establish arbitrary X/Z-phase equivalence or full physical-scenario migration.

## Local cost comparison

The source shrinks from 2,183 lines / 64,834 bytes to
295 lines / 47,320 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.609 s | 1.138 s |
| Link system | 2.411 s | 2.088 s |
| Emit C++ | 2.139 s | 2.067 s |
| Emit Verilog | 1.812 s | 1.554 s |
| Build C++ | 1.338 s | 1.179 s |
| Build Verilog | 1.548 s | 1.421 s |
| First measured full C++ run | 0.613 s | 0.342 s |
| First measured full Verilog run | 0.360 s | 0.293 s |
| Warm C++ median, one worker | 0.309257 s | 0.043230 s |
| Warm Verilog median | 0.035881 s | 0.034506 s |
| Warm native peak RSS median | 4.48 MiB | 3.33 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 10,185,154 bytes | 8,694,778 bytes |
| Generated C++ total | 2,687,303 bytes | 2,546,767 bytes |
| Generated Verilog total | 587,255 bytes | 479,642 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/enum_helpers --target cpp --cycles 184
pycircuit run examples/enum_helpers --target verilog --cycles 184
```
