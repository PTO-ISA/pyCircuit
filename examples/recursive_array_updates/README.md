# Recursive array updates

This example restores the second `recursive_array_updates` root and its
`update_recursive_arrays` rule from
`8887e6de:examples/agentic-circuit/blocks/bounded_integer_operations.py`.
The original independent oracle is `_recursive_expected` and
`test_recursive_and_wide_array_updates_match_all_backends` in
`f29de0d2:tests/integration/agentic-circuit/e2e/test_bounded_range_runtime.py`.
The bounded-conversion and full-u64 roots in that historical source are separate
examples.

`Request` contains an 8-bit raw index followed by an 8-bit replacement. The
combinational rule seeds three `ArrayInner` structs, five nominal enum values,
three two-bit range values and all 65 three-bit wide elements. Each struct keeps
its tag, nominal `ArrayMode` and ordinal. `ArrayMode` has an explicit one-bit
encoding with `IDLE=0` and `RUN=1`; ordinal and range values come from modulo
three, and wide values come from explicit low-three-bit slices.

Recursive zero initialization supplies the initial enum values and Table shapes.
Existing Table maps seed every struct, range and wide element without repeating
65 literals. These Tables are local values and add no persistent storage.

The rule makes four independent copies and performs real indexed updates:
structs at `raw_index % 3`, enums at `raw_index % 5`, ranges at
`raw_index % 3`, and wide elements at `raw_index % 65`. A fifth copy sets wide
element zero to seven. The outputs read the original, updated and chained Tables
separately. Updating the copies preserves the source; changing the chained copy
preserves the first wide update.

`RecursiveArrayResult` is one 42-bit payload in the original field order:

| Field | Bits | Observation |
| --- | ---: | --- |
| `source_struct_tag` | 8 | Original struct at the selected struct index |
| `updated_struct_tag` | 8 | Replacement struct tag |
| `source_struct_mode` | 1 | Original struct's nominal mode |
| `updated_struct_mode` | 1 | Updated struct's nominal mode |
| `source_enum` | 1 | Original enum at the selected enum index |
| `updated_enum` | 1 | Updated enum at the selected enum index |
| `source_range` | 2 | Original range at the selected struct index |
| `updated_range` | 2 | Updated range at the selected struct index |
| `source_wide` | 3 | Original wide Table at the selected wide index |
| `updated_wide` | 3 | Updated wide Table at the selected wide index |
| `wide_first` | 3 | Updated wide Table at index zero |
| `wide_last` | 3 | Updated wide Table at index 64 |
| `chained_selected` | 3 | Chained wide Table at the selected wide index |
| `updated_after_chain` | 3 | First wide update after the chained copy changes |

`RecursiveArrayUpdates` accepts `valid`, `data: Request` and `take`, returning
input `ready`, output `valid` and the typed result. One request queue feeds one
result queue. Both have depth one, latency one and the `downstream_pop` policy.
An input accepted at edge E0 transfers into the result queue at E1 and can first
retire at E2. The pipeline holds two tokens, supports simultaneous head pop and
replacement, and has no empty flow-through. Stalled output payloads hold their
values and order.

Work observes old queue state; successful whole-system checking precedes Xfer.
Queues start empty. Reset empties them, held clocks transfer nothing, and failed
epochs commit no state. The native/RTL host oracles drive physical clocks and
resets explicitly and compare equivalent precommit observations. The closed
`@system` bench uses the compiler's generated clock/reset scaffolding.

`bench.py` uses the four original requests and independent literal records,
checks every result field, and checks accepted/observed counts and pipeline
draining over 16 cycles. It offers the requests consecutively with output
readiness asserted. The independent native/RTL drivers retain the historical
sequence that waits for each result before sending the next request, alongside
additional raw-index, replacement and queue-timing coverage.

| Raw index | Replacement | Original packed 42-bit result |
| ---: | ---: | ---: |
| 0 | 9 | 624955961 |
| 2 | 14 | 35322946742 |
| 64 | 5 | 1099869737325 |
| 255 | 131 | 4389679644635 |

Build using the candidate compiler/Runtime installation from this checkout:

```sh
cmake -S examples/recursive_array_updates \
  -B .pycircuit_out/recursive-array-updates -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PWD/.pycircuit_out/recursive-array-install"
cmake --build .pycircuit_out/recursive-array-updates --parallel 4
ctest --test-dir .pycircuit_out/recursive-array-updates \
  --output-on-failure --no-tests=error
```

Native workers one/two and Verilator pass 4,669 Work samples. The independent
oracle constructs all four arrays, copies and updates them, and checks every
result field. It retains the original four golden cases and sequential request
history, adds 2,048 raw/replacement combinations and identity cases, and checks
65-lane index boundaries, stalls, full replacement, held clocks and reset.
The closed system also passes both backends for 16 cycles (32 sampling epochs,
64 observations and 18 source-check definitions).

[Generated excerpts](GENERATED.md) and the [artifact receipt](GENERATED.json)
bind the module's execution and source/artifact fingerprints.

```sh
pycircuit run examples/recursive_array_updates --target cpp --cycles 16
pycircuit run examples/recursive_array_updates --target verilog --cycles 16
```

The generic record-projection simplifier now retains verified aggregate
projections when equivalent hardware types have different source provenance.
It does not mutate types or add an aggregate conversion. This allows the existing
zero-initialization/map representation without repeating 65 seed literals.

The historical oracle is known-value-only and establishes no original X/Z
acceptance. Additional raw/replacement coverage does not exhaust every input
combination. This example does not establish migration of other historical roots
or a full nightly pass.
