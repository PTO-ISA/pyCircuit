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

The two CTest gates run actual generated native workers1/2, Verilator and genuine
Icarus:173 known Work samples and239 native/Icarus four-state samples. The oracle
uses a two-entry input deque and separate result slot. Two complete fill/drain
rounds, depth-two pointer wrap, full replacement, held levels, changed offers,
reset, all carry lengths,65534/65535 and alternating values are covered.

Known history is68 accepted/63 retired/5 reset-dropped; four-state history is
93/85/8. Both finish empty with observed peak occupancy3. Unknown-token stalls
and a rising reset dropping3 unknown tokens are exercised. All runners have
finite limits. IR checks confirm exactly two owners with depths2/1 and the
reviewed availability/head-read/ready policies.