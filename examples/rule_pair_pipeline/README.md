# Independent buffered rule pair

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

`PairResult` retains both complete 64-bit results: right data at bits 0 through
63, right valid/ready at 64/65, left data at 66 through 129 and left valid/ready
at 130/131, for 132 bits total. Any X/Z operand bit makes its arithmetic result
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
build artifacts outside the source tree. Both gates also run through the aggregate examples build.

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
The separate `bench.py` system checks a finite regular-clock known-state scenario, including queue saturation, stalls, replacement, boundary values and final drain. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/rule_pair_pipeline --target cpp --cycles 158 --build-dir .pycircuit_out/rule_pair_pipeline/system-cpp
pycircuit run examples/rule_pair_pipeline --target verilog --cycles 158 --build-dir .pycircuit_out/rule_pair_pipeline/system-verilog
```
