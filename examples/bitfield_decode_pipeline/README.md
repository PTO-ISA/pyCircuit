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

The module and four-state CTest gates run generated native workers 1/2, Verilator and genuine
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

## Scenario bench

`bench.py` keeps the existing 20-field, 266-bit `Stimulus` type and stores the
184 original scenarios in an immutable typed Table. Row 184 is all zero;
`stimulus(phase)` selects that row for every known u16 input from 184 through
65,535. This helper fallback is distinct from the system counter, which holds
at 183. Repeated rows and overwritten input fields remain explicit scenario
data; the Table does not recompute expected DUT results.

The complete system retains its original eight-field packet constructor, three
ordered DUT inputs, ten unconditional assertions and eleven observations.
`advance(phase)` still registers before `check()`. The run keeps its original
184-cycle limit.

Independent source checks compare all 1,310,720 field values across the complete
known-u16 helper domain, including the zero fallback. Twenty complete native
and RTL traces match all 4,048 observations and the terminal result after
excluding only changed source-position metadata; native workers 1 and 2 are
included. Baseline and compact versions each pass the module, system and
four-state gates, 3/3. See the module [excerpts and receipt](GENERATED.md).

The closed system covers a regular-clock known-state trajectory. Original
physical-clock, midstream-reset and four-state histories remain with the
unchanged independent module drivers; this source proof does not establish
arbitrary X/Z-phase equivalence or complete physical-scenario migration.

## Local cost comparison

The source shrinks from 3,370 lines / 97,800 bytes to
304 lines / 69,518 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, emit, build
and first-run times are single serial observations; warm medians use three
alternating full-length pairs. Every fresh process includes initialization.
These measurements do not guarantee results on other platforms or optimization
levels.

| Measurement | Expanded cases | Compact source |
| --- | ---: | ---: |
| Compile bench | 2.597 s | 1.695 s |
| Emit C++ | 3.500 s | 3.021 s |
| Emit Verilog | 2.664 s | 2.188 s |
| Build C++ | 1.813 s | 1.436 s |
| Build Verilog | 2.247 s | 1.484 s |
| First measured full C++ run | 0.765 s | 0.160 s |
| First measured full Verilog run | 0.353 s | 0.329 s |
| Warm C++ median, one worker | 0.474555 s | 0.045366 s |
| Warm Verilog median | 0.041123 s | 0.038501 s |
| Warm native peak RSS median | 5.92 MiB | 4.02 MiB |

| Artifact size | Expanded cases | Compact source |
| --- | ---: | ---: |
| Final IR | 16,543,776 bytes | 13,217,155 bytes |
| Generated C++ total | 4,082,973 bytes | 3,633,669 bytes |
| Generated Verilog total | 908,489 bytes | 689,308 bytes |

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/bitfield_decode_pipeline --target cpp --cycles 184
pycircuit run examples/bitfield_decode_pipeline --target verilog --cycles 184
```
