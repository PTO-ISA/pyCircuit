# Nested enum payload pipeline

## Payload behavior

Packet is 26 bits: nested Header(opcode6, Mode2), payload17 and matched1.
Explicit Mode width2 preserves the historical codes IDLE=0, RUN=1 and WAIT=2.
The rule copies the input, sets header.mode to RUN, and compares the original
input mode with WAIT for matched. Opcode and payload remain unchanged. The
original input alias is an immutable value snapshot; the nested update does not
change the later comparison.

The Boolean matched field is represented physically as `ac.u1`.
Its comparison remains Boolean and converts at the explicit field boundary.
This does not add general logical Boolean struct fields. Matched is payload data,
independent of the outer transport valid signal.

Invalid enum codes and X/Z are ordinary payload carriers. No membership filter,
repair or runtime assertion is added. Untouched fields preserve value/known/Z;
overridden fields become their declared known constants. Equality returns known
false when a known bit conflicts, and X when unresolved. The new source uses
nominal Enum values and ordinary record copy/field updates through the existing
frontend and common IR.

`EnumPayloadPipeline(valid, data, take)` returns typed Result(ready, valid, data).
Input data has the record type above; output retains the same type.

## Original timing

The historical typed input and rule result each owned a depth1/latency1 queue.
The current module retains both with explicit `downstream_pop` ready policy,
matching historical full replacement. Returning the result adds no third queue.
E0 captures an input, E1 commits the transformed token, and E2 is its earliest
consumption. Neither pure rule evaluation nor a copied local adds storage.

## Build and verification

```sh
cmake -S examples/enum_payload_pipeline -B /absolute/build/enum_payload_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/enum_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/enum_payload_pipeline --output-on-failure --no-tests=error
```

The module and four-state CTest gates execute the actual generated native DUT with workers 1/2,
Verilator and genuine Icarus. Each checks 603 known Work samples and another
51 four-state Work samples in native/Icarus. Tests cover every payload bit,
all opcode/mode codes including invalid ones, retained and overridden X/Z,
known-conflict versus unresolved equality, and transport of false/X matched data.
Full replacement, stalls, changed rejected offers, held high/low clocks, reset
and drain use a separate two-slot oracle and actual-output retirement ledger.

Known history is 290 accepted, 286 retired, 4 reset-dropped, zero outstanding, peak two slots.
The four-state history is 14 accepted, 10 retired, 4 reset-dropped, zero
outstanding, peak two. The computed unknown comparison bit has no asserted
value plane; copied wire fields retain exact value/known/Z planes. All runs
have finite limits, and test vectors are generated procedurally.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
12-field, 56-bit `Stimulus` retains the original schema, field order and complete
input/expected payloads. Row 184 is all zero, preserving the helper fallback
for every known u16 phase from 184 through 65,535. This is distinct from the
system counter, which holds at 183. The run still lasts 184 cycles.

Stimulus stays a flat Bits record. The system still maps input mode 0 to IDLE,
1 to RUN and every other code to WAIT, then constructs the nested Header/Packet.
The original active inputs remain modes 0/1/2; this rewrite does not expand them
to code 3. `enum_to_bits` checks/logs and the overwritten incoming matched field
remain unchanged. Matched is payload, separate from transport valid.

The complete advance and system bodies are unchanged, including unconditional
assertions, log order and advance-before-check registration. Independent source
checks compare all 786,432 field values across the known-u16 helper domain.
Twenty complete native and RTL traces match all 2,576 observations and the
terminal result after excluding only changed source-position metadata, including
native workers 1 and 2. Baseline and compact versions each pass all three
standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical drivers retain 603 known and 51 four-state Work samples,
with their reset, held-clock, conservation and original-history checks. The
existing four-state CTest executes Icarus; no supplemental test path is needed.
The regular-clock system does not replace those scenarios, and known-u16 source
proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 2,001 lines / 58,066 bytes to
282 lines / 42,291 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.427 s | 1.065 s |
| Link system | 2.139 s | 1.904 s |
| Emit C++ | 2.654 s | 1.856 s |
| Emit Verilog | 1.584 s | 1.407 s |
| Build C++ | 1.387 s | 1.259 s |
| Build Verilog | 1.625 s | 1.597 s |
| First measured full C++ run | 0.642 s | 0.432 s |
| First measured full Verilog run | 0.589 s | 0.263 s |
| Warm C++ median, one worker | 0.278734 s | 0.031092 s |
| Warm Verilog median | 0.028443 s | 0.026988 s |
| Warm native peak RSS median | 4.14 MiB | 3.09 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 9,472,404 bytes | 7,689,682 bytes |
| Generated C++ total | 2,383,981 bytes | 2,155,444 bytes |
| Generated Verilog total | 528,990 bytes | 420,039 bytes |

The warm and full-run measurements improve here; the figures are local observations, not a universal speedup.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/enum_payload_pipeline --target cpp --cycles 184
pycircuit run examples/enum_payload_pipeline --target verilog --cycles 184
```
