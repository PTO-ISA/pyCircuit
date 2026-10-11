# Buffered record increment rule

RuleToken retains one unsigned16-bit value. The pure rule copies the token and
increments its value modulo65536, including65535 -> 0. RulePipeline accepts
valid/data/take and returns typed ready/valid/data (18 packed result bits).
Clock/reset and transaction proposals remain hidden from Python.

## Exact buffer contract

There are two queue owners but **three token slots**: the original explicit
input queue has depth2, and the inferred rule-result queue has depth1. Both
have availability latency1, combinational head reads, no empty bypass and
explicit downstream-pop readiness. The sink adds no queue.

For committed occupancy n0/n1, downstream take t, and committed input head:

```text
result_ready = (n1 == 0) or (n1 != 0 and t)
advance = (n0 != 0) and result_ready
input_ready = (n0 < 2) or advance
result_data = (old_input_head.value + 1) modulo 65536
```

E0 captures input, E1 commits its transformed result, and E2 is earliest output
consumption. A stalled output can fill all three slots. Full simultaneous
replacement must consume old heads, preserve FIFO ordering and retain total
occupancy. Held clocks do not transfer; rising reset drops all queued tokens.

This plain-rule path has no module-family boundary or instance-result FIFO and
is independent of the historical module-backend divergence. Original native
UInt arithmetic was two-state. Historical RTL/current native addition produces
all-X arithmetic data for any X/Z operand; tests must not assert an incidental
value plane for that unknown result. Zero/all-X data remain valid tokens.
Retired pre-Work offers have different timestamp behavior from combinational
RTL ready; the current driver uses the hardware edge contract and a separate
commit-qualified token ledger.

## Build and verification

```sh
cmake -S examples/rule_pipeline -B /absolute/build/rule_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/rule_pipeline --parallel 4
ctest --test-dir /absolute/build/rule_pipeline --output-on-failure --no-tests=error
```

The module and four-state CTest gates run actual generated native workers1/2, Verilator and genuine
Icarus:173 known Work samples and239 native/Icarus four-state samples. The oracle
uses a two-entry input deque and separate result slot. Two complete fill/drain
rounds, depth-two pointer wrap, full replacement, held levels, changed offers,
reset, all carry lengths,65534/65535 and alternating values are covered.

Known history is68 accepted/63 retired/5 reset-dropped; four-state history is
93/85/8. Both finish empty with observed peak occupancy3. Unknown-token stalls
and a rising reset dropping3 unknown tokens are exercised. All runners have
finite limits. IR checks confirm exactly two owners with depths2/1 and the
reviewed availability/head-read/ready policies.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
6-field, 36-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 184 is all zero, retaining the helper fallback
for every known u16 phase from 184 through 65,535. That fallback is distinct
from the system counter, which holds at 183. The run still lasts 184 cycles.

The nominal RuleToken constructor and complete u16 value remain intact. The
input D2 and result D1 queues still provide three token slots with one increment
and E2 earliest retirement. The bench does not collapse the storage boundary.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 393,216 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,472
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 173 known Work samples and 239 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,198 lines / 28,766 bytes to
245 lines / 20,344 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 0.857 s | 0.561 s |
| Link system | 1.364 s | 1.001 s |
| Emit C++ | 1.216 s | 0.921 s |
| Emit Verilog | 1.009 s | 0.767 s |
| Build C++ | 1.014 s | 0.847 s |
| Build Verilog | 1.559 s | 1.386 s |
| First measured full C++ run | 0.578 s | 0.741 s |
| First measured full Verilog run | 0.528 s | 0.663 s |
| Warm C++ median, one worker | 0.169237 s | 0.017712 s |
| Warm Verilog median | 0.017559 s | 0.016008 s |
| Warm native peak RSS median | 3.11 MiB | 2.47 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 5,378,169 bytes | 4,014,157 bytes |
| Generated C++ total | 1,408,747 bytes | 1,137,154 bytes |
| Generated Verilog total | 311,763 bytes | 240,001 bytes |

Recorded first-run regressions: native 0.578 → 0.741 s, RTL 0.528 → 0.663 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/rule_pipeline --target cpp --cycles 184
pycircuit run examples/rule_pipeline --target verilog --cycles 184
```
