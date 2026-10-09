# Bounded integer operations

This example restores the first `bounded_integer_operations` root from
`8887e6de:examples/agentic-circuit/blocks/bounded_integer_operations.py` through
the current Python capture, MLIR, common IR and C++/Verilog flow. Its original
independent oracle is `_expected` and
`test_conversions_and_dynamic_array_match_all_backends` in
`f29de0d2:tests/integration/agentic-circuit/e2e/test_bounded_range_runtime.py`.
The recursive-array and full-u64 roots in the historical source are separate
examples.

`Request` contains five unsigned bytes in declaration order and an unsigned
8-bit raw index. The decode wraps and clamps that full index, performs checked
selection, and computes all 23 historical results. A failed range check returns
the lower bound and a false flag; it still produces a result. Narrow output
slices occur after the complete checks and arithmetic.

The local Table algorithm saves the source value, makes an indexed update in a
copy, and then sets element zero to 99 in a second copy. Results observe all
three snapshots. Neither update changes the source or the first updated copy.

| Output | Bits | Meaning |
| --- | ---: | --- |
| `wrapped` | 3 | Raw index modulo 5 |
| `saturated` | 4 | Raw index clamped to [4, 8] |
| `checked_value` | 3 | Raw index if below 5, otherwise 0 |
| `checked_valid` | 1 | Raw index below 5 |
| `advanced` | 3 | Wrapped index plus 1 |
| `selected` | 8 | Source at checked index |
| `wrapped_two` | 1 | Raw index modulo 2 |
| `wrapped_eight` | 3 | Raw index modulo 8 |
| `wrapped_full` | 8 | Original raw index |
| `narrow_selected` | 8 | Source at the raw index's low two bits |
| `zero_selected` | 8 | Source at index zero |
| `checked_window_value` | 4 | Raw index in [4, 8], otherwise 4 |
| `checked_window_valid` | 1 | Raw index in [4, 8] |
| `below_upper` | 1 | Wrapped index below 5 |
| `cross_domain_ordered` | 1 | Saturated value greater than wrapped index |
| `restored` | 3 | Advanced index minus 1 |
| `updated_selected` | 8 | First copy at checked index |
| `updated_first` | 8 | First copy at index zero |
| `updated_last` | 8 | First copy at index four |
| `source_after_update` | 8 | Source at checked index after copying |
| `chained_first` | 8 | Second copy at index zero |
| `chained_selected` | 8 | Second copy at checked index |
| `updated_after_chain` | 8 | First copy at checked index after copying again |

`BoundedIntegerOperations` accepts input `valid`, `data: Request`, and
`take: Ready`. Its `Result` contains input `ready`, 23 named output-valid bits,
and the 23 named payload fields. Grouping those ports does not merge their
storage: one input queue feeds 23 independent output queues. Every queue has
depth one, latency one and the `downstream_pop` policy. A shared conjunction
allows the input token to transfer into all outputs atomically when every
output can accept. Individual consumers may drain their own channels while
another channel stalls. No next token partially refills the drained channels.
A simultaneous head pop and replacement preserves order and occupancy.

Queues start empty and have no empty flow-through. Work observes old queue
state; successful whole-system checking precedes Xfer. Reset empties all queues,
a held clock transfers nothing, and a failed epoch commits no state. Host
oracles drive explicit physical clock/reset levels and compare the matching
pre-edge RTL observation. The closed `@system` bench uses compiler-generated
clock/reset scaffolding.

`bench.py` contains the original seven raw values (0, 3, 4, 5, 8, 9, 255), the
original payload (11, 22, 33, 44, 55), and independently specified literal
expected records. Its 96-cycle run stalls all output consumers at residues two
and three modulo seven, checks every output field and validity, counts accepted
and consumed tokens, and checks that the pipeline drains. This compact bench
covers common readiness. Independent native and RTL drivers own the broader
per-channel timing and raw-index tests.

Build with a Runtime/compiler installation produced from this checkout:

```sh
cmake -S examples/bounded_integer_operations \
  -B .pycircuit_out/bounded-integer-operations -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PWD/.pycircuit_out/bounded-integer-install"
cmake --build .pycircuit_out/bounded-integer-operations --parallel 4
ctest --test-dir .pycircuit_out/bounded-integer-operations \
  --output-on-failure --no-tests=error
```

The independent module oracle passes 1,071 Work samples on native workers one
and two and Verilator. It retains the original seven requests and 96-cycle
history, covers all 256 raw-index values with varied payloads, and exercises
asymmetric consumption across all 23 output channels, full replacement,
held clocks, and reset while fully or partly occupied. All channel histories
must conserve accepted, retired and reset-discarded tokens independently.

The closed system also passes native workers one/two and Verilator for 96 cycles
(192 sampling epochs, 384 observations and 49 source-check definitions).
[Generated excerpts](GENERATED.md) and the [artifact receipt](GENERATED.json)
bind the module's actual execution and source/artifact fingerprints.

```sh
pycircuit run examples/bounded_integer_operations --target cpp --cycles 96
pycircuit run examples/bounded_integer_operations --target verilog --cycles 96
```

The historical oracle uses known values. It provides no original X/Z acceptance
claim, and exhaustive raw-index testing cannot exhaust all possible 40-bit
payloads. This example does not establish migration of the other historical
roots or a full nightly pass. The DUT shares one decode computation and one
atomic fanout condition; its 24 queue instances represent required independent
storage, rather than repeated arithmetic networks.
