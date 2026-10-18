# Masked opcode decode pipeline

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named ingress and
egress queues retain the two depth-one, latency-one `downstream_pop` owners.
One registered `transfer` Rule pops the old ingress head, computes the payload
and pushes the egress atomically; the compiler supplies availability and capacity
guards. Raw `in_valid`/`in_data` and `out_ready` bindings retain the external
handshake, and `ready`/`valid`/`data` expose raw queue boundary views. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

Instruction contains opcode4 and is_compute1. The ordinary typed `decode` helper
copies the instruction and replaces is_compute with the wildcard predicate
`1xx0`: `(opcode & 9) == 8`.
It preserves the original opcode, including X/Z. Only opcode bits 3 and 0
participate in the predicate; X/Z in the two middle bits is ignored. A known
conflicting checked bit makes the comparison false even when another checked
bit is unknown; an unresolved comparison produces X. This is not enumeration
of the matching known codes, which would change four-state behavior.

The old Boolean result field is explicitly represented by `ac.u1`; the predicate
remains a Boolean computation and converts at the field boundary. This does not
add general Boolean struct fields or broaden the accepted Match selector
profile. The field is payload data, separate from transport valid.

`MaskedDecodePipeline(valid, data, take)` returns Result(ready, valid, data),
with a 5-bit Instruction payload and 7-bit total result. The source allocates
exactly two depth1/latency1 queues using explicit `downstream_pop`, preserving
the original implicit input and rule-result queues and full replacement.
Returning the output adds no storage. E0 captures an input, E1 commits its
classified token, and E2 is its earliest consumption. Work observes old state;
stalls fill at most two slots, held clocks do not transfer, and rising reset
empties both queues. Python contains no physical clock/reset/proposal wires.

```sh
cmake -S examples/masked_decode_pipeline -B /absolute/build/masked_decode_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/masked_decode_pipeline --parallel 4
ctest --test-dir /absolute/build/masked_decode_pipeline --output-on-failure --no-tests=error
```

The module and four-state CTest gates run native workers 1/2, Verilator and genuine Icarus against
an independent two-slot oracle: 97 known Work samples cover every opcode and
both prior flag values. Four native cases per worker and four Icarus cases
check ignored-middle X/Z, checked-bit uncertainty, known-conflict false results,
and preservation of the original opcode planes. Tests include full replacement,
stalls, changing rejected offers, held high/low levels, bubbles, reset and drain.
The commit-qualified ledger is 38 accepted, 36 retired, 2 reset-dropped, zero
outstanding, peak two slots; all runners have finite limits.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
8-field, 14-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 184 is all zero, retaining the helper fallback
for every known u16 phase from 184 through 65,535. That fallback is distinct
from the system counter, which holds at 183. The run still lasts 184 cycles.

The original opcode and incoming is_compute field remain explicit, even though
the DUT overwrites classification. The wildcard predicate and its ignored middle
bits stay in the DUT; the bench does not replace it with equality to one opcode.
Classification remains payload, separate from protocol valid.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 524,288 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,840
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 97 known Work samples and 4 full-DUT X/Z cases.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,306 lines / 32,479 bytes to
259 lines / 23,050 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 0.936 s | 0.647 s |
| Link system | 1.494 s | 1.179 s |
| Emit C++ | 1.332 s | 1.111 s |
| Emit Verilog | 1.088 s | 0.889 s |
| Build C++ | 1.061 s | 0.907 s |
| Build Verilog | 1.523 s | 1.391 s |
| First measured full C++ run | 0.721 s | 0.791 s |
| First measured full Verilog run | 0.706 s | 0.688 s |
| Warm C++ median, one worker | 0.175187 s | 0.021725 s |
| Warm Verilog median | 0.020868 s | 0.019774 s |
| Warm native peak RSS median | 3.27 MiB | 2.64 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 6,038,646 bytes | 4,967,689 bytes |
| Generated C++ total | 1,531,299 bytes | 1,411,967 bytes |
| Generated Verilog total | 337,777 bytes | 284,283 bytes |

Recorded first-run regressions: native 0.721 → 0.791 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
For the recorded comparison, DUTs, drivers, configuration, cycle limits and
timeouts were unchanged.

```sh
pycircuit run examples/masked_decode_pipeline --target cpp --cycles 184
pycircuit run examples/masked_decode_pipeline --target verilog --cycles 184
```
