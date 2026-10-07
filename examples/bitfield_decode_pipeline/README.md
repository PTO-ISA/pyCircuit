# Instruction bitfield extraction and update

## Payload behavior

DecodeItem retains its original field order and widths: word32, opcode6,
opcode_rd11, immediate17, mode3, rd5, low25, updated32, totaling 131 bits.
The rule copies the item and replaces four fields:

- opcode receives `word[26:32]`.
- opcode_rd receives `word[21:32]`.
- immediate receives `word[4:21]`.
- updated receives `ac.concat(word[26:32], rd, word[4:21], mode, word[:1])`.

word, mode, rd and the independent low25 field stay unchanged. In particular,
low25 is not recomputed from word. Incoming opcode/opcode_rd/immediate/updated
are overwritten. Ordinary record copy and field assignments preserve untouched
fields; slices and concat preserve raw value/known/Z without mask/OR's Z loss.

## Original timing

Exactly two depth1/latency1 queues preserve the inferred input queue and
rule-result queue. Both explicitly select `downstream_pop` for original full
replacement. Returning the result adds no third queue; no empty bypass exists.
E0 captures input, E1 commits the result stage, and E2 is earliest consumption.
All Work samples use old state. Held clocks do not transfer; stalls fill at most
two slots; rising reset clears both stages. Python leaves clock/reset/proposal
wires hidden. The host drives the physical pins through the existing typed DUT.

## Build and verification

```sh
cmake -S examples/bitfield_decode_pipeline -B /absolute/build/bitfield_decode_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/bitfield_decode_pipeline --parallel 4
ctest --test-dir /absolute/build/bitfield_decode_pipeline --output-on-failure --no-tests=error
```

The two CTest gates run generated native workers 1/2, Verilator and genuine
Icarus. They compare 305 known Work rows and 575 native/Icarus four-state
Work rows. Known one-hot vectors and isolated X/Z at every input position,
plus dense patterns with nonzero latent values, check all three native planes.
Icarus checks visible X/Z placement. The two-slot oracle includes startup,
full replacement, stalls, changing rejected offers, active held high/low clocks,
bubbles, drain and reset. All runners have explicit finite limits.

Known accepted/retired/reset-dropped counts are 141/137/4; four-state counts are
276/272/4. Both histories finish empty, with peak occupancy two. Final IR checks
confirm two D1 queues, availability latency1, combinational head-read latency0,
explicit downstream replacement, no empty flow and no additional state owners.