# Unsigned 64-bit calculator

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

## Validation

The standalone module and system tests use the public compile/link/emit route,
native workers 1/2 and Verilator. The consumer target uses the existing Runtime
and is registered in examples nightly; it is not an installed SDK target.
See [generated excerpts](GENERATED.md) and [verification inputs](GENERATED.json).
The no-division source variant is historical diagnostic evidence. Current
acceptance covers the complete division-bearing source and its known-value oracles.

## Scenario bench

`bench.py` exports `example_calculator.bench.ExerciseCalculator`. Its immutable Table contains
237 original rows of a 4-field, 72-bit `Scenario`.
All 237 rows remain, with the complete 64-bit index clamped to row 236:
`(key=0, key_press=0, expected_display=18446744073709551606,
expected_op_pending=0)`. The key stays five bits. Integer values retain every
u64 bit, including UINT64_MAX; no floating-point conversion or recomputed golden
is used. Decimal overflow, AC, pending operations and divisor-zero substitution
retain their original stimulus and expected values.

The phase remains a wrapping `bits[64]` counter initialized to zero. Only Table
selection clamps. The original DUT call, input/expected bindings, unmasked
assertions, messages, logs and check-before-advance registration remain intact.
The generated system still runs 237 cycles with identical low/high sampling inputs.

Independent source proof preserves all 948 active field values and the complete
known-u64 domain by splitting original constant comparison cells across candidate
row selections and checking the tail. Original partitions need not have one cell
per Table row; repeated rows remain distinct. High-phase probes supplement this
proof rather than replacing it. No arbitrary X/Z internal-phase equivalence is
claimed; initialization and the unchanged increment keep this closed phase known.

Baseline and compact versions each pass both existing standalone tests. Twenty
complete native/RTL system traces preserve all 948 observations and the terminal
result, including native workers 1/2, after excluding only changed source-position
metadata. Regular-clock systems do not replace physical clock/reset histories.

The unchanged physical driver retains 474 known Work samples. This example
has no existing RTL four-state macro or dedicated X/Z oracle; no new four-state
or example-wide failure/zero-commit coverage is claimed.

See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,660 lines / 65,769 bytes to
291 lines / 13,524 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.282 s | 0.410 s |
| Link system | 1.774 s | 0.747 s |
| Emit C++ | 1.621 s | 0.702 s |
| Emit Verilog | 1.351 s | 0.583 s |
| Build C++ | 1.083 s | 0.815 s |
| Build Verilog | 1.428 s | 2.004 s |
| First measured full C++ run | 0.730 s | 0.349 s |
| First measured full Verilog run | 0.355 s | 0.289 s |
| Warm C++ median, one worker | 0.252963 s | 0.022297 s |
| Warm Verilog median | 0.012300 s | 0.012039 s |
| Warm native peak RSS median | 3.23 MiB | 2.34 MiB |

| Artifact size | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Final IR | 7,495,932 bytes | 2,747,867 bytes |
| Generated C++ total | 1,518,755 bytes | 902,154 bytes |
| Generated Verilog total | 315,560 bytes | 187,686 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/calculator --target cpp --cycles 237
pycircuit run examples/calculator --target verilog --cycles 237
```
