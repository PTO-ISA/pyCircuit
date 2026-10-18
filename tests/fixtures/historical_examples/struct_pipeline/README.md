# Two-stage record update pipeline

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named queue handles
retain the external `in_valid`/`in_data` and `out_ready` bindings, with raw
`ready`/`valid`/`data` boundary views.
One registered `transfer` Rule pops, computes and pushes atomically.
Ingress depth 2 and egress depth 2, both latency one, retain the
`downstream_pop` ready policy.
The compiler supplies availability and capacity guards. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

Both original `int` fields resolve to unsigned fixed 64-bit hardware. `Item`
therefore keeps `value:ac.u64` followed by `remaining:ac.u64`. The typed pure
`update_item` rule reconstructs the complete record, incrementing value and
decrementing remaining independently modulo 2^64. Value UINT64_MAX wraps to
zero; remaining zero underflows to UINT64_MAX. Neither field's arithmetic
carries or borrows into the other field.

The complete 128-bit payload packs value at bits 64 through 127 and remaining
at bits 0 through 63. `StructResult` packs valid at bit 128 and ready at bit 129,
for 130 bits total. Expected output is `(value_out << 64) | remaining_out`;
both outputs derive from the original input fields.

Two explicit queues each have depth 2 and availability latency 1, for four
complete tokens of capacity. Both declare `downstream_pop` readiness, read old
committed heads and have no empty bypass. The sink adds no storage. E0 captures
an input, E1 commits its transformed old head to the result queue, and E2 is
the earliest external retirement. Depth 2 adds capacity without a mandatory
extra latency stage.

For old input/result occupancies n0/n1, result readiness is `n1 < 2` or an
old result-head pop. Advance consumes only the old input head when result
readiness permits. Input readiness is `n0 < 2` or that advance. Full replacement
retains both occupancies and FIFO order through depth-two pointer wraps.
Held clock levels do not commit despite changed offers/take; rising reset
clears up to four tokens. Physical clock/reset levels are host-driven and
absent from Python; the pure rule adds no state or transaction proposal.

Any X/Z in value makes only the computed value field 64 X bits. A known
remaining field still decrements exactly. Any X/Z in remaining similarly
makes only remaining X while a known value increments exactly. Both uncertain
fields yield 128 X bits. Computed unknown fields have no Z and unspecified
hidden value planes. Neither field is an unchanged raw transport field;
zero and uncertain tokens remain valid.

## Build route

Use the accepted installed toolchain and public example CMake helper. It
compiles the source independently, links the explicit unit closure and emits
both targets from one verified final artifact. Run from the repository root:

```sh
cmake -S examples/struct_pipeline -B /absolute/build/struct_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/struct_pipeline --parallel 4
ctest --test-dir /absolute/build/struct_pipeline --output-on-failure --no-tests=error
```

Keep build artifacts outside the source tree. The module and four-state gates also run through the
aggregate examples build.

## Verification

Actual generated native workers 1/2 and Verilator agree on 8,989 known Work
samples. Genuine full-DUT Icarus agrees with these and 1,151 four-state Work
samples. The independent oracle uses 64-bit ripple carry/borrow and two
old-state depth-two logical deques, with a separate actual-DUT transaction
ledger. 4,455 known stimulus tokens and 536 native latent X/Z variants
cover the arithmetic; no exhaustive 64-bit or 128-bit claim is made.

The known matrix contains all 65×65 carry/borrow pairs, independent field
boundaries, packed-bit one-hots across the 63/64 boundary and mixed full-width
peers. Four-state tests isolate each field's uncertainty, preserving exact
known arithmetic in its peer, and exercise both fields uncertain together.
Computed unknown fields must be all X with no Z; only their latent values are
unconstrained.

Both streams exercise E0/E1/E2, full four-token stalls and replacements,
multiple wraps of both queue pointers, active held clocks with changed offers
and reset, two rising resets dropping four tokens each, drain and recovery.
Known history is 4476 accepted / 4468 retired / 8 reset-dropped; four-state 557 / 549 / 8. Every run ends empty, reaches peak occupancy four
and conserves all accepted tokens. Runner limits are 12,000 ticks.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
8-field, 260-bit `Stimulus` retains the original schema, field order and complete
input/expected payloads. Row 184 is all zero, preserving the helper fallback
for every known u16 phase from 184 through 65,535. This is distinct from the
system counter, which holds at 183. The run still lasts 184 cycles.

All four input/expected data columns retain exact unsigned 64-bit integers.
Value and remaining stay separate fields; increment and decrement are not packed
128-bit arithmetic. The DUT still has two depth-two queues and capacity four,
with independent carry/borrow checks. The 12,000-tick physical oracle budget and
all 8,989/1,151 known/four-state samples remain unchanged.

The complete advance and system bodies are unchanged, including unconditional
assertions, log order and advance-before-check registration. Independent source
checks compare all 524,288 field values across the known-u16 helper domain.
Twenty complete native and RTL traces match all 1,840 observations and the
terminal result after excluding only changed source-position metadata, including
native workers 1 and 2. Baseline and compact versions each pass all three
standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical drivers retain 8,989 known and 1,151 four-state Work samples,
with their reset, held-clock, conservation and original-history checks. The
existing four-state CTest executes Icarus; no supplemental test path is needed.
The regular-clock system does not replace those scenarios, and known-u16 source
proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,557 lines / 44,322 bytes to
251 lines / 32,600 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.110 s | 0.827 s |
| Link system | 1.777 s | 1.475 s |
| Emit C++ | 1.551 s | 1.283 s |
| Emit Verilog | 1.269 s | 1.042 s |
| Build C++ | 1.105 s | 1.003 s |
| Build Verilog | 1.537 s | 1.534 s |
| First measured full C++ run | 3.386 s | 0.258 s |
| First measured full Verilog run | 0.145 s | 0.255 s |
| Warm C++ median, one worker | 0.221628 s | 0.022020 s |
| Warm Verilog median | 0.021663 s | 0.020444 s |
| Warm native peak RSS median | 3.58 MiB | 2.70 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 7,193,529 bytes | 5,414,172 bytes |
| Generated C++ total | 1,858,040 bytes | 1,517,115 bytes |
| Generated Verilog total | 415,476 bytes | 313,591 bytes |

The first measured Verilog run rises from 0.145 to 0.255 s despite a lower warm median. Keep that single-run cost separate from warmed execution.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/struct_pipeline --target cpp --cycles 184
pycircuit run examples/struct_pipeline --target verilog --cycles 184
```
