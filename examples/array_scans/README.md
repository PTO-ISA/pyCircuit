# Ordered array scans

This example restores the final `array_scans` root, `scan_arrays` rule,
`subtract` helper and `ScanResult` from
`8887e6de:examples/agentic-circuit/blocks/array_combinators.py`.
The original independent oracle is
`test_ordered_scan_prefixes_match_all_backends` in
`f29de0d2:tests/integration/agentic-circuit/e2e/test_array_combinator_runtime.py`.
The array-combinator and reduction roots in that historical source are separate
examples.

`Request` contains three unsigned bytes in declaration order: `first`, `second`
and `third`. One ordered loop visits their three-element Table and computes all
four historical scans. Every prefix stores the accumulator after that step;
all arithmetic wraps to eight bits at each operation.

- Subtraction starts at zero and subtracts each value through the `subtract`
  helper. Its nonassociative sequence remains ordered.
- Addition starts at ten and adds each value. The initial value contributes once.
- Resetting addition starts at five. A zero input resets the accumulator to zero;
  any other input adds to the previous accumulator.
- The nominal `Pair` accumulator starts with `(first, first)`. Each step resets
  its first field to zero for a zero input, or adds the input to its previous
  first field. Its second field remains unchanged.

`Prefixes` contains three-element Tables for each scalar scan and for complete
`Pair` values. The source makes local Table copies from a zero-initialized
`Prefixes` value and stores all four actual prefixes at each iteration. Earlier
pair snapshots remain intact after later accumulator updates. These temporary
values add no persistent storage or pipeline stages. The loop body contains
only local computations and updates; assertions and logs belong to the bench.

The result is one 64-bit `ScanResult` in the original declaration order:

| Field | Bits | Observation |
| --- | ---: | --- |
| `subtract_first` | 8 | Subtraction prefix after the first input |
| `subtract_second` | 8 | Subtraction prefix after the second input |
| `subtract_third` | 8 | Subtraction prefix after the third input |
| `nonzero_last` | 8 | Addition prefix after the third input |
| `reset_second` | 8 | Resetting-addition prefix after the second input |
| `reset_third` | 8 | Resetting-addition prefix after the third input |
| `tuple_first` | 8 | First pair prefix's first field |
| `tuple_third` | 8 | Third pair prefix's first field |

`ArrayScans` accepts input `valid`, `data: Request` and output `take`, returning
input `ready`, output `valid` and the typed result. One request queue feeds one
result queue. Both have depth one, latency one and the `downstream_pop` policy.
An input accepted at edge E0 transfers to the result queue at E1 and can first
retire at E2. The two-token pipeline preserves order, holds stalled results,
and supports simultaneous head pop and replacement without empty flow-through.

Work observes old queue state. Successful whole-system checking precedes Xfer;
a failed epoch commits no state. Queues start empty, reset empties them, and a
held clock transfers nothing. Native/RTL host oracles drive physical clocks and
resets explicitly and compare equivalent precommit observations. The closed
`@system` bench uses the compiler's generated clock/reset scaffolding.

`bench.py` exports `example_array_scans.bench.ArrayScansSystem`. Its 16-cycle
run offers the four original requests consecutively with output readiness
asserted, checks all eight fields against independently specified literal
records, counts four accepted and consumed tokens, and checks that the pipeline
drains. The independent native/RTL drivers retain the original sequence that
waits for each result before sending the next request, plus additional numeric
and queue-timing coverage.

| Request | Original packed 64-bit result |
| --- | ---: |
| (0, 0, 0) | 42949672960 |
| (1, 2, 3) | 18446174595540779527 |
| (255, 1, 2) | 72336921615400449 |
| (7, 9, 11) | 18010146857285586466 |

Build with the candidate compiler/Runtime installation from this checkout:

```sh
cmake -S examples/array_scans -B /absolute/build/array_scans -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/array_scans --parallel 4
ctest --test-dir /absolute/build/array_scans --output-on-failure --no-tests=error
```

The standalone verification compares 1,031 Work samples across native workers
one and two and Verilator. Its history accepts 495 tokens, retires 492 and drops
three during directed resets, checking conservation and two-token capacity.
The closed system verifies 16 cycles (32 sampling epochs) on both backends.
[Generated artifacts](GENERATED.md) records source and output identities for
these runs; the catalog entry does not establish full framework acceptance.

The historical runtime oracle uses known values and supplies no root-specific
X/Z acceptance. This root exposes eight selected prefix fields; separate generic
loop tests must check every intermediate snapshot and unchanged pair second
field. This example does not establish other root migrations, exhaustive input
coverage or a full nightly pass.
