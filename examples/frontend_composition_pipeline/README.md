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
generated design. Standalone and targeted aggregate gates each pass 2/2.
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