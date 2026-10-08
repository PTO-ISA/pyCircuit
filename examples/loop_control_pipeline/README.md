# Loop control pipeline

`LoopControlPipeline(valid, data, take)` returns Result8: ready7, valid6 and
LoopToken6. Source D2/L1, feedback D1/L1 and result D1/L1 retain four payload slots
and24 logical payload bits. One stateless Step module expresses combinational
connections and adds no storage or cycle. Boolean fields use explicit
u1 physical storage, without claiming logical Boolean nominal-field roundtrips.

## Timing, masking and state

Feedback takes priority over the source head. A continuation advances even with
a full result queue; only exit requires output space. A resident feedback exit
cannot process a new source head on that edge, although free source slots can
still accept external offers. Explicit downstream_pop preserves full replacement
where a head is consumed. The feedback input-ready does not feed its own take,
avoiding a ready cycle without extra buffering.

E0 accepts into an empty source. remaining0 or a known stop exits at E1 and first
retires at E2. Otherwise remaining=n produces n updates, exit at E(n+1) and first
retirement at E(n+2). Held/falling clocks and discarded proposals do not advance
state. Rising reset drops occupied tokens, and invalid output is packed zero.
The host/SystemRunner drives compiler-generated hidden clock/reset pins; the
shared Runtime owns Work/Xfer and discard behavior.

Known stop masks unknown remaining and exits the entire token unchanged.
remaining0 masks unknown stop and likewise exits unchanged. Unknown skip never
affects continuation and retains all raw value/known/Z planes. An unmasked
unknown continuation fails before any owner commits; a poisoned queued head
requires Reset after terminal executor failure. Successful outputs preserve raw
flags and masked-exit fields; a completed iterative remaining value is known0.

The old parser's artificial `(not skip)==(not skip)` condition could differ on
X/Z before old PYC self-comparison folding. The rewrite preserves the documented
tail-continue no-op and does not claim equivalence to that unoptimized artifact.
For these exact inputs, accepted continuation starts with known remaining0..15
and strictly decreases it; break only shortens execution. The old1024 guard and
iteration bookkeeping are therefore unreachable/unobservable and omitted under
the independent policy.
This does not admit arbitrary loops or establish a general iteration-limit check.

## Build and verification

```sh
cmake -S examples/loop_control_pipeline -B /absolute/build/loop_control_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/loop_control_pipeline --parallel 4
ctest --test-dir /absolute/build/loop_control_pipeline --output-on-failure --no-tests=error
```

Independent tests must cover all64 known tokens, all remaining/stop/skip symbol
combinations with native latent alternatives, current masking/failure rules,
four-slot saturation, feedback priority, blocked continuation/exit, exact timing,
holds/reset/discard and actual accept/retire/drop ledgers. Internal iterations
are modeled independently and checked through visible timing and output values.
Standalone gates pass2/2 and the paired targeted aggregate passes4/4. Native
workers1/2 and Verilator match1,272 known Work rows; native and genuine Icarus
match19,458 four-state rows. All64 known inputs execute. Of4,096 symbolic tokens,
1,096 are safe and3,000 have an effective unknown continuation: both native
latent variants give2,192 successful cases and6,000 terminal cases per worker,
with Reset between failures. After the failure batch and each protocol-failure
class, a known remaining1 token must execute at the exact E0/E1/E2/E3 schedule,
retire exactly once and leave an empty pipeline. RTL executes every successful symbol twice plus
seven isolated fatal classes; it does not claim6,000 separate RTL failure runs.

Known/extended history accepts111 tokens, retires107 and reset-drops4, finishing
empty at peak occupancy four. The isolated four-state sequence accepts/retires
2,192 without drops. Tests check continuation with a full output, blocked exits,
same-edge source suppression, replacements, holds and busy reset. Three owner
probes include an actual exact-once drain after discarded host Reset. Initial
RTL testbench lexical whitespace and an Icarus isunknown(concat) false positive
were repaired without changing DUT or expected values; explicit per-bit guards
were separately shown to reject injected X/Z. The final recovery-probe repair
was followed by complete standalone and targeted two-test reruns. Failed and
superseded records remain retained, and all original Work traces are unchanged.

## Generated system usage

`bench.py` exports `example_loop_control_pipeline.bench.ExerciseLoopControlPipeline`. Compile
`loop_control_pipeline.py`, then `bench.py`, import the published DUT interface, and link
that explicit system root through the public compile/link/emit flow. Run
`pycircuit run examples/loop_control_pipeline --target cpp --cycles 635` or select
`--target verilog`. Imported records use their original nominal declarations
and supply every field explicitly.

All original known-stream data edges are represented in this regular-clock
scenario, with fixed independent expectations from the retained native oracle
along a resetless trajectory. The 635 cycles include a final observation.
The original DUT, native/RTL drivers, finite configuration, and any four-state,
reset/discard, latency and token-ledger matrices remain unchanged. Physical
held-level and midstream-reset scenarios still require those original module
oracles; this system does not claim complete physical-scenario equivalence.
