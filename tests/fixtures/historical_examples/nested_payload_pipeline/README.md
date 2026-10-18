# Nested payload mode update

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named ingress and
egress handles retain the two depth-one, latency-one `downstream_pop` queues.
One registered `transfer` Rule pops the old ingress head, computes the payload
and pushes the egress atomically; the compiler supplies availability and capacity
guards. Raw `in_valid`/`in_data` and `out_ready` bindings retain the external
handshake, and `ready`/`valid`/`data` expose the queue boundary views. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

## Payload behavior

Packet contains Header(opcode6, mode3) followed by payload17, totaling26 bits.
The rule copies the input and sets only `result.header.mode = item.header.mode + 1`.
Mode wraps modulo8, including 7 -> 0; opcode and payload remain unchanged.
The new source declares Header before Packet and uses ordinary immutable record
copy/update rather than the retired with_fields API. The physical record layout
and every original field are preserved.

`NestedPayloadPipeline(valid, data, take)` returns typed ready/valid/data:
26 payload bits and 28 total result bits. Original native UInt arithmetic was
two-state. Historical RTL and current fixed-bit arithmetic produce X throughout
the arithmetic result if an operand contains X/Z; Z does not survive addition.
For the nested root, untouched opcode/payload preserve their complete raw planes
and unknown copied fields do not contaminate a known mode increment. Tests must
not constrain the value plane of an unknown computed result.

## Timing and ownership

Exactly two depth1/latency1 queues retain the original typed input and rule
result. Explicit `downstream_pop` preserves full replacement; head-read latency
is zero and empty flow is disabled. The returned sink adds no third queue.
E0 captures input, E1 commits the transformed token, and E2 is its earliest
consumption. All Work outputs use old committed state. Held clocks do not
transfer; stalls fill at most two slots; rising reset clears both queues.
Clock/reset and proposals remain absent from Python authoring.

## Build and verification

```sh
cmake -S examples/nested_payload_pipeline -B /absolute/build/nested_payload_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/nested_payload_pipeline --parallel 4
ctest --test-dir /absolute/build/nested_payload_pipeline --output-on-failure --no-tests=error
```

The module and four-state CTest gates execute actual generated native workers 1/2, Verilator and
genuine Icarus. They check 1121 known Work rows and 281 native/Icarus four-state
Work rows, including wraparound, zero/all-X tokens, stalls/full replacement,
changed offers, active held high/low clocks, reset and drain. Scalar cases cover
all256 byte values; nested cases cover all64 opcode values and all8 modes plus
walking input bits. Per-bit and dense X/Z patterns distinguish computed unknown
arithmetic from exact copied value/known/Z planes. All runners are finite.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
10-field, 56-bit `Stimulus` retains the original schema, field order and complete
input/expected payloads. Row 184 is all zero, preserving the helper fallback
for every known u16 phase from 184 through 65,535. This is distinct from the
system counter, which holds at 183. The run still lasts 184 cycles.

The explicit Header/Packet constructor retains opcode6, raw mode3 and payload17.
The three-bit mode is not the nominal two-bit Mode used by the enum example,
despite equal aggregate widths. Expected fields remain independent literal data;
the bench does not compute the DUT's mode increment or discard invalid payloads.

The complete advance and system bodies are unchanged, including unconditional
assertions, log order and advance-before-check registration. Independent source
checks compare all 655,360 field values across the known-u16 helper domain.
Twenty complete native and RTL traces match all 2,208 observations and the
terminal result after excluding only changed source-position metadata, including
native workers 1 and 2. Baseline and compact versions each pass all three
standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical drivers retain 1,121 known and 281 four-state Work samples,
with their reset, held-clock, conservation and original-history checks. The
existing four-state CTest executes Icarus; no supplemental test path is needed.
The regular-clock system does not replace those scenarios, and known-u16 source
proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,860 lines / 53,771 bytes to
269 lines / 39,272 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.315 s | 0.950 s |
| Link system | 2.034 s | 1.691 s |
| Emit C++ | 1.806 s | 1.588 s |
| Emit Verilog | 1.495 s | 1.238 s |
| Build C++ | 1.217 s | 1.095 s |
| Build Verilog | 1.515 s | 1.555 s |
| First measured full C++ run | 1.231 s | 0.307 s |
| First measured full Verilog run | 0.559 s | 0.252 s |
| Warm C++ median, one worker | 0.256079 s | 0.025797 s |
| Warm Verilog median | 0.024939 s | 0.024085 s |
| Warm native peak RSS median | 3.97 MiB | 2.89 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 8,851,296 bytes | 6,762,688 bytes |
| Generated C++ total | 2,209,042 bytes | 1,857,905 bytes |
| Generated Verilog total | 489,949 bytes | 371,227 bytes |

The single Verilog build measurement rises from 1.515 to 1.555 s even though native runtime and generated sizes decrease.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/nested_payload_pipeline --target cpp --cycles 184
pycircuit run examples/nested_payload_pipeline --target verilog --cycles 184
```
