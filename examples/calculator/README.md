# calculator

A u64 integer calculator driven by a 5-bit keypad, one key press per epoch.

## Interface

The Python module returns the named struct `CalcResult`; the generated DUT
exposes it as a single packed `result` port (66 bits), not as two separate ports.

| Struct field | Direction | Width | Meaning |
| --- | --- | --- | --- |
| `result.display` | out | u64 | committed display value |
| `result.op_pending` | out | u2 | latched op: 0 add, 1 sub, 2 mul, 3 div |
| `key` | in | u5 | keypad code |
| `key_press` | in | u1 | key valid this epoch |

Because the first declared field occupies the most significant bits, the packed
value is `{display, op_pending}` — `op_pending` is the low 2 bits.

Key codes: `0..9` digit, `10` add, `11` subtract, `12` multiply, `13` divide,
`14` equals, `15` clear.

## State and timing

Persistent state (all reset to 0): `lhs` u64, `rhs` u64, `op` u2, `in_rhs` u1,
`shown` u64 — five `dffe` leaves in the emitted RTL.

Outputs observe committed state during Work and are not recomputed after Xfer.
A key accepted on a rising-edge frame affects the next Work snapshot. The runner
passes the pre-Step index to `drive` and the completed count (index + 1) to
`sample`; the testbench accounts for this callback convention. RTL observes
settled data before applying each frame's edge, matching the native snapshot.

Behaviour per epoch:

1. A digit extends `lhs` while entering the left operand, or `rhs` after an
   operator key (`in_rhs`), multiplying by ten and adding the digit.
2. An operator key latches `op`, sets `in_rhs`, and starts a fresh `rhs`.
3. `computed` walks the historical mux chain from `lhs` through add, subtract,
   multiply and divide. A retained op therefore passes `lhs` through.
4. A zero divisor is replaced by one before the divide.
5. `equals` commits `computed` into `lhs` and re-arms entry; `clear` zeroes
   everything.

## Oracles

Two oracles share `driver.cpp`.

**Recovered historical idle oracle.** The original `tb_calculator.py` clocked
`clk`, reset with 2 asserted + 1 deasserted cycles, drove `key=0`,
`key_press=0`, expected `display == 0`, and called `finish(at=1)`. Its
`timeout(64)` was an upper bound that the run never reached: the historical
simulation executed the reset cycles plus two post-reset cycles and checked
`display == 0` once. This driver reproduces that shape — reset, then an idle
hold in which `display` must remain 0. The 64-epoch hold used here is a
**stronger extension**, not a byte-for-byte reproduction of the historical
cycle count.

**New independent functional oracle.** The original testbench had no functional
coverage, so an independent keypad model drives decimal
entry, `+ - * /`, equals, clear and the zero-divisor rule: `12+34=46`,
`7*6=42`, `9-4=5`, `8/0=8`, `100/4=25`. The original 242-frame
trace is retained, then extended with u64 maximum entry and division by three,
addition/subtraction/multiplication wrap, and decimal-entry overflow.
The complete trace contains 474 Work samples within the 512-tick runner limit.

The GUI demo's decimal/sign/ERROR expectations describe a different, non-u64
design and are deliberately **not** used as a golden.

## Reproduction

```sh
pycircuit compile -c calculator.py --source-root . \
  --package-prefix example_calculator -o unit
pycircuit link unit --top example_calculator.calculator.Calculator -o top.ac
pycircuit emit top.ac --target cpp     -o cpp
pycircuit emit top.ac --target verilog -o verilog
```

The driver implements the shared runner contract used by
`cmake/verify_example.py`:

```sh
./pycircuit_calculator --config config.json --workers 1
./pycircuit_calculator --config config.json --workers 2
```

It prints one `WORK <epoch> <display> <op_pending>` record per sampled epoch (474
records), then a `PASS` line. Serial and parallel `WORK` output is identical.
`config.json` supplies the finite `max_ticks` bound.

## Current validation status

The complete, division-bearing source now passes compile/link and C++/Verilog
emission with the current combinational runtime divider. The generated C++ plus
driver pass syntax checking, and the RTL/testbench pass Verilator lint.
The earlier one-epoch discrepancy came from testbench observation/callback
indices; neither backend's Work/Xfer or hardware state timing was changed.

The consumer `pycircuit_calculator` target is approved and registered in examples
nightly. It combines this driver and generated module sources with the existing
Runtime through `PycircuitExamples.cmake`; it is not installed/exported in the
framework SDK. See `GENERATED.json` for the current native workers 1/2 and RTL
verification receipt, and `GENERATED.md` for actual generated artifacts.

The original no-division variant is historical diagnostic evidence, not acceptance
of this source. Four-state and failure/zero-commit example coverage remain
unverified; the generic arithmetic/runtime tests cover those contracts separately.

## Generated system usage

`bench.py` exports `example_calculator.bench.ExerciseCalculator`. The explicit source
closure is `calculator.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/calculator --target cpp --cycles 237` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

All 237 original keypad cycles are represented, including 64 idle cycles,
decimal entry, all four operations, equals, clear, zero-divisor behavior and
the complete u64 overflow sequence. Fixed display/pending vectors come from
the original independent host model. The initial reset-window inputs are idle;
initial generated host Reset supplies the same starting state.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
