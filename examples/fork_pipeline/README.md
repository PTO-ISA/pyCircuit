# Decoupled fork pipeline

The original `int` is unsigned 64-bit transport. One depth-one input queue
feeds two depth-two output queues. All have latency one and explicit
`downstream_pop` readiness, preserving old-head full replacement without empty
bypass. Two ordinary one-bit variables remember which branches have received
the held input head. A branch can receive while its peer stalls; a branch that
already received that head must not receive it again. The input retires only
after both branches have received it. Completion clears both flags, including
when a new input replaces the completed head on that edge.

There are five physical queue slots and two metadata bits. The held input is
not another payload copy or an extra capacity reservation. At most three source
identities await delivery to either consumer; physical slots can contain two
copies of one identity. Reset accounting tracks each consumer's outstanding
obligations separately, including partially delivered heads.

`ForkPipeline(valid, data, left_take, right_take)` returns `ForkResult` with
ready at bit 130, left_valid at 129, left_data at 128:65, right_valid at 64,
and right_data at 63:0. Python exposes ordinary queues, variables and a rule;
the existing MLIR flow infers the two storage owners and hidden clock/reset.
The same verified IR feeds both backends. No fork primitive is added.

Outputs are the Work snapshot before Xfer. Rising reset empties all queues and
clears delivery metadata. Held and falling clock levels do not transfer. Empty
initialized data is zero, and copied payloads preserve all value/known/Z planes.
System checking precedes commit; failed systems require host Reset before reuse.

## Build and verification

```sh
cmake -S examples/fork_pipeline -B /absolute/build/fork_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/fork_pipeline --parallel 4
ctest --test-dir /absolute/build/fork_pipeline --output-on-failure --no-tests=error
```

The installed example helper independently compiles the source, links its
explicit closure, emits both backends, and runs the generated SystemRunner with
workers 1 and 2 against an independent token-obligation oracle. RTL checks use
Verilator and genuine Icarus four-state execution. Tests must distinguish partial
delivery from atomic fanout, prevent duplicates under asymmetric stalls, and
exercise replacement, wrap, reset, hold, discard and failure recovery.

The standalone generated-DUT gates pass: workers 1/2, Verilator and Icarus
agree on 395 known Work samples; native and genuine Icarus agree on another
659 four-state samples. Native raw transport checks all three planes, including
latent values. The four-state vectors include 256 single-bit uncertainty/latent
cases and eight dense patterns.

Each consumer's known history accepts 144, retires 137 and reset-drops seven
obligations; the four-state history accepts 276, retires 269 and drops seven.
Mirrored partial-reset cases drop 1/3 and 3/1 obligations, and full reset drops
3/3 at five occupied physical slots. The oracle requires partial deliveries,
duplicate suppression and replacement, with a bounded final drain. Direct-owner
discard/retry preserves metadata; the terminal unknown-control test checks
failure and sample rejection, followed by mandatory host Reset.

The terminal effective-transfer failure occurs before metadata preparation and therefore
does not directly observe metadata after a late system failure. The rollback
proof combines this lifecycle test, the direct-owner test that prepares and
discards all three queues plus two flag owners, the emitted common discard
path, and unchanged accepted Q6 late-parent/sibling failure tests. It does not
resume a failed system to inspect state or claim a new late-sibling execution.

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
9-field, 198-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. That fallback is distinct
from the system counter, which holds at 157. The run still lasts 158 cycles.

Left/right takes, valids and payload expectations remain independent. Partial
delivery and asymmetric drain scenarios are retained; the bench neither duplicates
a fast branch's delivery nor turns the design into atomic broadcast. The five
physical slots and two delivery flags remain unchanged.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 589,824 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,896
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 395 known Work samples and 659 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,298 lines / 36,223 bytes to
231 lines / 26,233 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 3.190 s | 0.651 s |
| Link system | 5.532 s | 1.204 s |
| Emit C++ | 4.371 s | 1.154 s |
| Emit Verilog | 3.756 s | 0.920 s |
| Build C++ | 1.154 s | 0.927 s |
| Build Verilog | 1.515 s | 1.424 s |
| First measured full C++ run | 0.692 s | 0.426 s |
| First measured full Verilog run | 0.613 s | 0.736 s |
| Warm C++ median, one worker | 0.164069 s | 0.023131 s |
| Warm Verilog median | 0.020243 s | 0.019685 s |
| Warm native peak RSS median | 3.45 MiB | 2.64 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 27,247,326 bytes | 4,924,280 bytes |
| Generated C++ total | 1,714,087 bytes | 1,450,694 bytes |
| Generated Verilog total | 376,010 bytes | 286,320 bytes |

Recorded first-run regressions: RTL 0.613 → 0.736 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/fork_pipeline --target cpp --cycles 158
pycircuit run examples/fork_pipeline --target verilog --cycles 158
```
