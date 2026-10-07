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