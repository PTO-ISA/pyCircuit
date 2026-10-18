# Aggregate payload rotation

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named queue handles
retain the external `in_valid`/`in_data` and `out_ready` bindings, with raw
`ready`/`valid`/`data` boundary views.
One registered `transfer` Rule pops, computes and pushes atomically.
Ingress depth 1 and egress depth 1, both latency one, retain the
`downstream_pop` ready policy.
The compiler supplies availability and capacity guards. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

The 28-bit packet contains an ordered Pair(first: 3 bits, second: 5 bits), a real four-element
Table of four-bit lanes, and selected: 4 bits. Pair members increment modulo 8/32;
lanes rotate from source indices 1,2,3,0; selected reads source lane 2. Ordered
Pair fields preserve the historical heterogeneous tuple positions. This example
does not establish a general tuple authoring API.

Two depth-one queues with downstream_pop replacement retain input and transformed
tokens. E0 captures, E1 transfers the transformation, and E2 consumes it. Work
reads old committed state. Held clocks cannot transfer; rising reset discards
both occupied slots; a full pipeline can replace both tokens during consumption.

The independent host and RTL tests preserve the original owners at revision
b3e52fb22061350d61b23089c8bed36913650245:

- test_queue_codegen.py native vector: pair(5,17), lanes(1,2,3,4), selected0
  produces pair(6,18), lanes(2,3,4,1), selected3, packed 220345363. Five
  Work/Xfer ticks retire exactly one token.
- test_pyc_backend.py backend vector: pair(5,29), lanes(1,2,3,4), selected15
  produces pair(6,30), lanes(2,3,4,1), selected3, packed 232928275. Ten cycles
  reset at 0, inject at 1 and keep take high; the sole post-edge output is cycle 2.
  The active Work sampler reevaluates at the low clock level after the rising
  commit to expose that post-edge state without an additional transfer.

New independent coverage uses all 256 pair combinations with varied lanes,
walking bits, zero-result tokens, full replacement, changed stalled offers,
held high/low clocks, reset discard and drain. Native workers 1/2 and Verilator
compare 645 known Work rows. Native workers 1/2 and genuine Icarus compare 329
four-state Work rows. The host checks complete value/known/Z planes for rotated
lanes and selected; each arithmetic field becomes all X if that field contains
X/Z. Tests leave only the latent value of computed unknown arithmetic unconstrained.
Copied X/Z bits retain their latent value planes exactly.

Build and run through the shared example framework:

```sh
cmake -S examples/aggregate_payload_pipeline -B /absolute/build/aggregate_payload_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/aggregate_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/aggregate_payload_pipeline --output-on-failure --no-tests=error
```

Run these commands to reproduce the module, system and four-state checks.

## Scenario bench

`bench.py` stores all 48 original scenarios in an immutable typed Table. Its
18-field, 60-bit `Stimulus` retains the original schema, field order and complete
input/expected payloads. Row 48 is all zero, preserving the helper fallback
for every known u16 phase from 48 through 65,535. This is distinct from the
system counter, which holds at 47. The run still lasts 48 cycles.

The constructor preserves Pair first3/second5, input lane order 0/1/2/3 and the
independent incoming selected field. Lane rotation and selection happen only in
the DUT; inputs are not pre-rotated or replaced by expected results. Each output
lane retains its own indexed assertion. The original execution histories above
remain separate verification assets.

The complete advance and system bodies are unchanged, including unconditional
assertions, log order and advance-before-check registration. Independent source
checks compare all 1,179,648 field values across the known-u16 helper domain.
Twenty complete native and RTL traces match all 960 observations and the
terminal result after excluding only changed source-position metadata, including
native workers 1 and 2. Baseline and compact versions each pass all three
standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical drivers retain 645 known and 329 four-state Work samples,
with their reset, held-clock, conservation and original-history checks. The
existing four-state CTest executes Icarus; no supplemental test path is needed.
The regular-clock system does not replace those scenarios, and known-u16 source
proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 845 lines / 23,076 bytes to
162 lines / 16,205 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 0.616 s | 0.508 s |
| Link system | 1.067 s | 0.973 s |
| Emit C++ | 0.956 s | 0.996 s |
| Emit Verilog | 0.817 s | 0.756 s |
| Build C++ | 0.891 s | 0.906 s |
| Build Verilog | 1.456 s | 1.539 s |
| First measured full C++ run | 0.170 s | 0.279 s |
| First measured full Verilog run | 0.124 s | 0.262 s |
| Warm C++ median, one worker | 0.038469 s | 0.014390 s |
| Warm Verilog median | 0.012745 s | 0.012549 s |
| Warm native peak RSS median | 2.69 MiB | 2.41 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 3,985,778 bytes | 3,339,472 bytes |
| Generated C++ total | 1,033,884 bytes | 987,215 bytes |
| Generated Verilog total | 240,356 bytes | 199,340 bytes |

Single C++ emission/build and first native/RTL run costs increase despite the lower native warm median and smaller artifacts. The table retains these regressions; this is not an across-the-board improvement.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/aggregate_payload_pipeline --target cpp --cycles 48
pycircuit run examples/aggregate_payload_pipeline --target verilog --cycles 48
```
