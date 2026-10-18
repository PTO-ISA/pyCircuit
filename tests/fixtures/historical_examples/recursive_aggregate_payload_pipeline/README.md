# Recursive aggregate payload update

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

The 13-bit Packet preserves the original ordered fields: Tagged(Mode1, value3),
Nested(Header(code3), value3), a real two-element Table of nominal Mode values,
and flag1. The update sets the tagged Mode to RUN, increments header.code modulo
8, swaps the two Modes, and derives flag from the original first Mode being IDLE.
Tagged.value and Nested.value remain unchanged. Ordered nominal Structs retain
the historical tuple positions without claiming a general tuple authoring API.

Two depth-one queues use downstream_pop replacement. E0 captures, E1 transfers
the transformation, and E2 consumes it. All Work reads use old committed state.
Held clocks cannot transfer; a rising reset discards both occupied tokens.

Both original execution owners at revision
b3e52fb22061350d61b23089c8bed36913650245 use packed 2962 → packed 7125:
tagged(IDLE,5) → (RUN,5), nested(code6,2) → (code7,2), Modes(IDLE,RUN) →
(RUN,IDLE), flag0 → flag1. The native test_queue_codegen.py history takes five
Work/Xfer ticks and retires exactly one result. The test_pyc_backend.py history
uses ten cycles, reset at 0, injection at 1, take always high, and the sole
post-edge output at cycle 2. The active sampler reevaluates at the low clock
level after the rising commit to expose that state without another transfer.

New independent coverage spans all eight header codes, all eight nested peer
values and all four Mode pairs, with varied tagged values/Modes and incoming
flags. Walking bits, header wrap, changed stalled offers, full replacement,
held clocks, reset discard and drain supplement the originals. The host and
Verilator check 615 known Work rows; native workers 1/2 and genuine Icarus
check 209 four-state Work rows. Per-bit and dense X/Z patterns distinguish exact
copied value/known/Z planes from the computed header and comparison flag.
Header arithmetic containing X/Z becomes all X; an unknown original Mode 0 makes
the flag X. Only those computed unknown latent values are unconstrained.

```sh
cmake -S examples/recursive_aggregate_payload_pipeline -B /absolute/build/recursive_aggregate_payload_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/recursive_aggregate_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/recursive_aggregate_payload_pipeline --output-on-failure --no-tests=error
```

Run these commands to reproduce the module, system and four-state checks.

## Scenario bench

`bench.py` stores all 48 original scenarios in an immutable typed Table. Its
18-field, 30-bit `Stimulus` retains the original schema, field order and complete
input/expected payloads. Row 48 is all zero, preserving the helper fallback
for every known u16 phase from 48 through 65,535. This is distinct from the
system counter, which holds at 47. The run still lasts 48 cycles.

The complete Tagged/Nested/Header/Packet constructors remain explicit. All three
nominal Mode conversions still select RUN for input one and IDLE otherwise;
the input modes tuple remains in order 0/1. The overwritten tagged mode and flag
stay in the stimulus. All three `enum_to_bits` check/log pairs remain intact,
and the bench does not pre-swap the DUT's two Mode elements.

The complete advance and system bodies are unchanged, including unconditional
assertions, log order and advance-before-check registration. Independent source
checks compare all 1,179,648 field values across the known-u16 helper domain.
Twenty complete native and RTL traces match all 960 observations and the
terminal result after excluding only changed source-position metadata, including
native workers 1 and 2. Baseline and compact versions each pass all three
standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical drivers retain 615 known and 209 four-state Work samples,
with their reset, held-clock, conservation and original-history checks. The
existing four-state CTest executes Icarus; no supplemental test path is needed.
The regular-clock system does not replace those scenarios, and known-u16 source
proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 719 lines / 20,561 bytes to
170 lines / 14,842 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 0.535 s | 0.482 s |
| Link system | 0.931 s | 0.927 s |
| Emit C++ | 0.825 s | 1.157 s |
| Emit Verilog | 0.726 s | 0.715 s |
| Build C++ | 0.855 s | 0.896 s |
| Build Verilog | 1.452 s | 1.480 s |
| First measured full C++ run | 0.155 s | 0.277 s |
| First measured full Verilog run | 0.136 s | 0.349 s |
| Warm C++ median, one worker | 0.033620 s | 0.014461 s |
| Warm Verilog median | 0.012783 s | 0.012899 s |
| Warm native peak RSS median | 2.55 MiB | 2.38 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 3,397,959 bytes | 3,128,756 bytes |
| Generated C++ total | 897,636 bytes | 965,342 bytes |
| Generated Verilog total | 211,378 bytes | 195,063 bytes |

Generated C++ grows from 897,636 to 965,342 bytes, and its single emission time rises from 0.825 to 1.157 s. First native/RTL runs and build costs also increase; warm RTL rises slightly from 0.012783 to 0.012899 s. These costs remain visible alongside the lower native warm median.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/recursive_aggregate_payload_pipeline --target cpp --cycles 48
pycircuit run examples/recursive_aggregate_payload_pipeline --target verilog --cycles 48
```
