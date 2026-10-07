# Leading and trailing zero-count pipeline

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

Standalone and targeted aggregate gates each pass 2/2. Native workers 1/2,
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