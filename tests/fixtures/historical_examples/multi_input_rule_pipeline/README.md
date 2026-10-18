# Atomic buffered input sum

The two ingress queues retain depth two and the result queue retains depth
one; all use latency one and `downstream_pop` policy. One registered `transfer`
Rule pops both old heads, adds them and pushes the result atomically.
Named queue handles retain external input/output bindings and raw
`ready`/`valid`/`data` boundary views. The two independent input-ready values are
separate tuple results; one `Channel` groups result valid and data.

Original `int` resolves to unsigned fixed 64-bit hardware. The inline sum of
the old left and right heads therefore wraps modulo 2^64, including MAX+1 to
zero. The source retains two D2/L1 input queues and one inferred D1/L1 result
queue, now explicit. All three use `downstream_pop` readiness, old committed
head reads and no empty bypass; the sink adds no storage.

The join consumes one left and one right head atomically when both are
available and result room exists. The cohesive `transfer` Rule expresses that
operation directly; compiler-owned Queue admission proves both pops and the push
before committing any of them.

The nth accepted left pairs with the nth accepted right. Inputs can arrive
independently; an absent peer or stalled result prevents both input pops. A
newly captured peer cannot bypass its queue into a same-edge sum. When both
contributors arrive at E0, the result commits at E1 and can retire at E2.
Full result retirement permits atomic replacement while either full input
queue may also replace. Held clock levels commit nothing despite changed
offers/take. Host-driven rising reset clears all three owners; Python adds
no clock/reset or transaction proposals.

There are five physical scalar storage slots: left two, right two, sum one.
A sum retains two contributor identities, so the fully occupied state contains
six accepted contributors. For each stream, accepted contributors equal retired
pairs plus reset-dropped contributors plus its input occupancy plus result
occupancy. Reset drops left-input-plus-result identities on the left and
right-input-plus-result identities on the right. A ledger that counts a sum
only once cannot prove contributor conservation.

The public result tuple is `left_ready`, `right_ready`, then `Channel`; the
channel packs sum at bits0 through63 and valid at64. Any X/Z in either operand
makes all64 sum bits X with no Z and unspecified hidden value plane. Data
uncertainty never changes the atomic control; zero and unknown sums remain valid
tokens.

## Build route

From the repository root, use the accepted installed toolchain:

```sh
cmake -S examples/multi_input_rule_pipeline -B /absolute/build/multi_input_rule_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/multi_input_rule_pipeline --parallel 4
ctest --test-dir /absolute/build/multi_input_rule_pipeline --output-on-failure --no-tests=error
```

The public example helper compiles this source independently, links the complete
explicit unit closure and emits both targets from one verified final artifact.
Keep outputs outside the source tree. The module and four-state gates also run through the aggregate examples build.

## Verification

Generated native workers 1/2 and Verilator agree on 8,751 known Work samples;
genuine full-DUT Icarus also agrees with 1,173 four-state samples. The independent
oracle uses ripple addition, two depth-two input deques, one result slot and
separate left/right contributor identities. Distinct asymmetric operands,
carry boundaries, independent arrival skew, blocked results, held clocks,
full replacement, input pointer wrap, resets and recovery are exercised.
Every input bit sees X/Z with both latent values, separately and alongside
uncertainty in its peer. All 67 output bits are checked; only computed unknown
sum latent values are unconstrained.

Actual-DUT ready/valid signals qualify accepted contributors and retired pairs.
Known runs accept 4347 contributors on each side, retire 4341 pairs and reset-drop
six contributors per side. Four-state runs accept 558 per side, retire 552 pairs
and reset-drop six per side. Every run finishes empty, reaches five physical
slots and six contributors, and checks independent per-stream conservation.
Limits are finite at 12,000 ticks. Final IR checks exactly the original
D2/D2/D1 owners and complete 64/67-bit interfaces, without extra instances or
collections. No framework change was needed.

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
9-field, 198-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. That fallback is distinct
from the system counter, which holds at 157. The run still lasts 158 cycles.

Both input valid/data channels and their separate readiness expectations remain
independent. The bench does not replace the atomic old-head join with current
input arithmetic or consume a missing peer. The original two D2 input queues
and D1 result retain five physical slots and their contributor accounting.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 589,824 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,580
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 8,751 known Work samples and 1,173 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,435 lines / 39,893 bytes to
237 lines / 28,748 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 3.342 s | 0.681 s |
| Link system | 5.771 s | 1.221 s |
| Emit C++ | 4.827 s | 1.138 s |
| Emit Verilog | 3.909 s | 0.936 s |
| Build C++ | 1.245 s | 0.932 s |
| Build Verilog | 1.662 s | 1.372 s |
| First measured full C++ run | 0.897 s | 2.732 s |
| First measured full Verilog run | 0.672 s | 0.850 s |
| Warm C++ median, one worker | 0.171031 s | 0.019263 s |
| Warm Verilog median | 0.018091 s | 0.017607 s |
| Warm native peak RSS median | 3.55 MiB | 2.66 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 28,849,810 bytes | 5,226,167 bytes |
| Generated C++ total | 1,771,892 bytes | 1,449,771 bytes |
| Generated Verilog total | 393,759 bytes | 293,839 bytes |

Recorded first-run regressions: native 0.897 → 2.732 s, RTL 0.672 → 0.850 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/multi_input_rule_pipeline --target cpp --cycles 158
pycircuit run examples/multi_input_rule_pipeline --target verilog --cycles 158
```
