# Independent buffered rule pair

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named queue handles
retain the external `in_valid`/`in_data` and `out_ready` bindings, with raw
`ready`/`valid`/`data` boundary views.
Separate registered `transfer_left` and `transfer_right` Rules each pop,
compute and push atomically, preserving independence between the two streams.
Each stream retains its two depth-one, latency-one `downstream_pop` queues.
The compiler supplies availability and capacity guards. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

The two streams are independent. Left tokens increment modulo 2^64; right
tokens multiply by 2 modulo 2^64. Original `int` means unsigned fixed 64-bit
hardware, represented here by `ac.u64`. Each one-use pure scalar rule is
inlined between its original input and result storage boundaries. Multiplication
remains arithmetic: replacing it with a shift would change X/Z behavior.

There are four D1/L1 queues, two per stream, with capacity two complete tokens
per branch and four total. Each branch has separate valid/data/take/ready,
old committed head reads, no empty bypass and explicit `downstream_pop`
readiness. The sinks add no queues. Missing or stalled left traffic cannot
block right progression, and vice versa. This design adds no join or arbitration.

Each branch captures at E0, transfers its transformed old head to the result
queue at E1 and permits earliest retirement at E2. Full simultaneous
replacement retains FIFO order and occupancy. Held clocks never commit despite
changed offers/takes; rising reset clears all four owners. Physical clock/reset
levels are host-driven and absent from Python. No transaction proposals or
unsupported scalar annotated-rule return are introduced.

The result is a typed `(left, right)` tuple. Each independent `Channel` output
retains one complete 66-bit boundary view: data at bits 0 through 63, valid at
64 and ready at 65. Any X/Z operand bit makes its arithmetic result
64 X bits with no Z, including a right high bit discarded by known doubling.
The known other stream remains exact and independent. Computed unknown hidden
value planes are unspecified; zero and unknown data remain real tokens.

## Build route

From the repository root, use the accepted installed toolchain:

```sh
cmake -S examples/rule_pair_pipeline -B /absolute/build/rule_pair_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/rule_pair_pipeline --parallel 4
ctest --test-dir /absolute/build/rule_pair_pipeline --output-on-failure --no-tests=error
```

The public example helper compiles the source independently, links its explicit
unit closure and emits both targets from one verified final artifact. Keep
build artifacts outside the source tree. The module and four-state gates also run through the aggregate examples build.

## Verification

Generated native workers 1/2 and Verilator agree on 531 known Work samples;
genuine full-DUT Icarus also agrees with 1,201 four-state samples. Independent
per-bit arithmetic and two separate old-state two-slot models check all 132
result bits. The known matrix covers carry boundaries, zero/MAX/high bits and
asymmetric stream data/rates. Each stream also sees every input bit as X/Z with
both latent values, dense uncertainty, stalls and recovery. Right bit63 X/Z
produces an entirely unknown arithmetic result, distinguishing multiply from
shift. Known peer results remain exact; computed unknown latent bits alone
are unasserted.

Actual-DUT ready/valid signals qualify per-stream identity ledgers. Known
left history is 232 accepted / 228 retired / 4 reset-dropped; right is
230 / 226 / 4. Four-state histories are 567 / 563 / 4 and 565 / 561 / 4.
All end empty, reach two slots per branch and four total, and conserve tokens.
Independent stalls, missing peers, full replacements, occupied resets, held
clocks with changed offers and E0/E1/E2 are checked. Limits are finite at
12,000 ticks. Final IR contains exactly four D1/L1 queues and no extra instances
or collections.

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
12-field, 264-bit `Stimulus` preserves the original schema, field order and all
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. This fallback is distinct
from the system counter, which holds at 157. The run retains 158 cycles.

The original if/elif priority chain becomes literal rows without merging either
stream. All six ordered valid/data/take arguments remain distinct, including
25 rows with different input-valid bits, 24 with different takes and 10 with
different output-valid flags. All four u64 columns remain exact. Left increment
and right multiplication stay in the DUT; the bench neither recomputes expected
results nor replaces multiplication by a shift.

The complete advance and system bodies are unchanged, including unconditional
assertions, observation order and advance-before-check/exercise registration.
Independent source checks compare all 786,432 field values across the known-u16
helper domain. Twenty complete native and RTL traces match all 2,212 observations
and the terminal result after excluding only changed source-position metadata,
including native workers 1 and 2. Baseline and compact versions each pass all
three standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical tests retain 531 known and 1,201 four-state Work samples,
with the reset, held-clock and conservation checks described above. The
registered four-state test already executes Icarus. Regular-clock systems do not
replace physical histories, and known-u16 source proof does not establish
arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,815 lines / 54,254 bytes to
247 lines / 39,553 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 4.416 s | 0.922 s |
| Link system | 7.545 s | 1.687 s |
| Emit C++ | 5.642 s | 1.590 s |
| Emit Verilog | 5.073 s | 1.265 s |
| Build C++ | 1.328 s | 1.127 s |
| Build Verilog | 1.567 s | 1.507 s |
| First measured full C++ run | 3.357 s | 2.135 s |
| First measured full Verilog run | 0.124 s | 3.145 s |
| Warm C++ median, one worker | 0.220987 s | 0.024754 s |
| Warm Verilog median | 0.024415 s | 0.022983 s |
| Warm native peak RSS median | 3.98 MiB | 2.88 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 36,886,278 bytes | 6,776,159 bytes |
| Generated C++ total | 2,279,011 bytes | 1,910,858 bytes |
| Generated Verilog total | 506,003 bytes | 374,180 bytes |

The first measured RTL run increases from 0.124 to 3.145 s despite the lower
warm median. That regression is retained separately; one observation does not
identify its cause or establish steady-state behavior.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/rule_pair_pipeline --target cpp --cycles 158
pycircuit run examples/rule_pair_pipeline --target verilog --cycles 158
```
