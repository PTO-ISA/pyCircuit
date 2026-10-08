# Masked opcode decode pipeline

Instruction contains opcode4 and is_compute1. The rule copies the instruction
and replaces is_compute with the wildcard predicate `1xx0`: `(opcode & 9) == 8`.
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

The two CTest gates run native workers 1/2, Verilator and genuine Icarus against
an independent two-slot oracle: 97 known Work samples cover every opcode and
both prior flag values. Four native cases per worker and four Icarus cases
check ignored-middle X/Z, checked-bit uncertainty, known-conflict false results,
and preservation of the original opcode planes. Tests include full replacement,
stalls, changing rejected offers, held high/low levels, bubbles, reset and drain.
The commit-qualified ledger is 38 accepted, 36 retired, 2 reset-dropped, zero
outstanding, peak two slots; all runners have finite limits.
The separate `bench.py` system checks a finite regular-clock known-state scenario with exact old-state output checks. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/masked_decode_pipeline --target cpp --cycles 184 --build-dir .pycircuit_out/masked_decode_pipeline/system-cpp
pycircuit run examples/masked_decode_pipeline --target verilog --cycles 184 --build-dir .pycircuit_out/masked_decode_pipeline/system-verilog
```
