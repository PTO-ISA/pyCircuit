# BF16 FMAC

`BF16Fmac(a_in:u16, b_in:u16, acc_in:u32, valid_in:u1)` returns
`FmacResult(result:u32, result_valid:u1)` in that order, for 33 packed bits.
Related stage fields share three named struct owners. Result data and valid
remain separate owners because their enables differ. The five owners retain
281 state bits, all initialized and reset to zero:

| Boundary | Owners | Bits |
| --- | ---: | ---: |
| S1: unpack, partial products and two CSA rounds | 1 | 147 |
| S2: remaining reduction and carry-select addition | 1 | 63 |
| S3: alignment and signed magnitude combination | 1 | 38 |
| Output: packed result and valid | 2 | 33 |

S1 retains six 16-bit row fields: four live reduction rows and two zero padding
rows, plus its four-bit row count. The shared 47-bit `Metadata` record travels
through S1 and S2. Product-zero and accumulator-zero
metadata remain at both S1 and S2, including the unused accumulator-zero flag.
The Python source contains no clock/reset or register primitive API. The host
drives generated physical sampling/reset pins.

Module scope composes pure typed helpers to compute each candidate from inputs or its
preceding stage's old Q. These calls describe combinational computations and add
no module instances or storage. `carry_save` serves six reduction sites;
`ripple8` serves all three carry-select chains; `right_barrel26` serves both
alignment paths and output normalization. The priority LZC and left barrel
remain explicit gate networks. Helpers do not call other rules or write owners.

One nested zero-argument `advance_fmac` Rule snapshots old output Q and old S3
valid before proposing its lexical Module owners. S1, S2 and S3 advance
on every rising edge, including invalid inputs. Output valid always copies
old S3 valid; output data updates only when old S3 valid is asserted and holds
otherwise. An input captured at E0 reaches output Q after E3 commits: four
register boundaries. Work returns preceding committed output Q, and Xfer does
not recompute the already sampled output. Existing whole-system checking and
discard govern state commit. An unknown output enable must fail checking and
discard every proposed owner update, rather than merging output data with old Q.

The arithmetic preserves the historical finite-bit algorithm:

- S1 flushes exponent-zero BF16/FP32 mantissas to zero and otherwise inserts the
  implicit one. Product exponent is ten-bit modular `a_exp + b_exp - 127`.
  Eight explicit AND partial products use eight-bit masks, widen before shifts,
  and reduce at sixteen bits through CSA rounds 8→6→4.
- S2 consumes old S1 rows and reduces 4→3→2. Its final adder remains an explicit
  eight-bit low ripple chain and two eight-bit high chains for carry-in zero/one,
  selected by the low carry. The final carry is discarded, as in the original.
  There is no whole-product multiply or adder primitive substitution.
- S3 normalizes once on product bit 15, compares only the low eight exponent
  bits, widens the product to 26 bits before shifting left by nine, and caps
  alignment at 26. Static barrel layers shift the smaller magnitude. Same-sign
  addition occurs at 27 bits before slicing to 26; opposite signs subtract the
  smaller magnitude, with product sign winning a magnitude tie. Product-zero
  selects the accumulator fields.
- S4 retains the six-bit, 26-step priority-mux leading-zero chain and both
  five-layer left/right normalization barrels with static shifts 1/2/4/8/16.
  Exponent adjustment wraps at ten bits; packing uses its low eight exponent
  bits and the low 23 fraction bits. A zero mantissa packs positive zero.

The explicit AND/OR/XOR gates, selection order and pre-shift widths preserve
the original network. Zero-OR widening/packing expressions remain where the
historical circuit used them; they are not replaced with raw Z-preserving
transport. This source does not claim complete four-state execution coverage.
The algorithm does not implement IEEE fused arithmetic, rounding, NaN/Inf
handling or subnormal arithmetic, and it must not be compared with a host
floating-point FMA as its oracle.

Compile `bf16_fmac.py` independently with `pycircuit compile`, link the explicit
unit closure selecting `BF16Fmac`, and emit C++ or Verilog from the same verified
final artifact. Separate typed-DUT/SystemRunner testbenches and the existing
example helper own execution with finite limits. The historical testbench
drove zero/invalid input and supplied no arithmetic oracle. The new test uses direct
8×8 integer multiplication and highest-set-bit normalization, independently of
the DUT gate network: 52 arithmetic cases, 10 literal anchors and 169 Work frames.
It checks four register boundaries, valid holes, output hold, repeated clock levels,
reset with three in-flight inputs and drain. Native workers 1/2 and RTL agree: 56 inputs are accepted, 53 retire and three
are dropped by reset. These unchanged testbenches remain the regression for the
refactor. The existing `bf16_fmac` driver additionally runs 32 cases in each worker mode: four known anchors,
24 partial-X/Z operand cases and four unknown-enable failure/reset-recovery cases. The shared verifier also compares its `FRAME`/`FAILURE`/`CASES` records between
workers. This partial-X/Z coverage remains bounded; it is not an exhaustive
four-state or IEEE arithmetic claim.

## Build and inspect

```sh
cmake -S examples/bf16_fmac -B /absolute/build/bf16_fmac -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/bf16_fmac --parallel 2
ctest --test-dir /absolute/build/bf16_fmac --output-on-failure --no-tests=error
```

## Scenario bench

`bench.py` exports `example_bf16_fmac.bench.ExerciseBF16Fmac`. Its immutable Table contains
86 original rows of a 6-field, 98-bit `Scenario`.
All 86 rows remain, with the complete 64-bit index clamped to row 85:
`(a_in=0, b_in=0, acc_in=0, valid_in=0, expected_result=3308764270,
expected_result_valid=0)`. The seven identical final rows 79–85 remain present.
The finite-bit goldens preserve four-register latency, nonzero invalid operands,
high result bits and held data; they are not replaced with host floating-point FMA.

The phase remains a wrapping `bits[64]` counter initialized to zero. Only Table
selection clamps. The original DUT call, input/expected bindings, unmasked
assertions, messages, logs and check-before-advance registration remain intact.
The generated system still runs 86 cycles with identical low/high sampling inputs.

Independent source proof preserves all 516 active field values and the complete
known-u64 domain by splitting original constant comparison cells across candidate
row selections and checking the tail. Original partitions need not have one cell
per Table row; repeated rows remain distinct. High-phase probes supplement this
proof rather than replacing it. No arbitrary X/Z internal-phase equivalence is
claimed; initialization and the unchanged increment keep this closed phase known.

Baseline and compact versions each pass both existing standalone tests. Twenty
complete native/RTL system traces preserve all 344 observations and the terminal
result, including native workers 1/2, after excluding only changed source-position
metadata. Regular-clock systems do not replace physical clock/reset histories.

The unchanged physical driver retains 169 known Work samples. In each native
worker mode, its 32 additional cases comprise four known anchors, 24 partial-X/Z
operand cases and four unknown-enable failure/reset-recovery cases. Complete raw
records match: 496 FRAME records, four FAILURE records and CASES 32. Native and
known RTL runs retain HISTORY 56 53 3: 56 accepted, 53 retired and three reset-dropped.

FRAME/FAILURE/CASES are native-only records. The shared verifier compares them
between workers, then filters them and HISTORY from its cross-backend WORK
comparison. This review therefore also compares complete raw baseline/candidate
records and exact HISTORY on both backends. There is no existing BF16 Icarus
macro or RTL four-state run. Native failure probes do not establish every owner's
internal zero-commit state, and this is not IEEE FMA or exhaustive four-state coverage.

See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,170 lines / 41,875 bytes to
144 lines / 10,299 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Compile bench | 0.874 s | 0.376 s |
| Link system | 1.630 s | 1.033 s |
| Emit C++ | 1.579 s | 0.931 s |
| Emit Verilog | 1.253 s | 0.702 s |
| Build C++ | 1.100 s | 1.019 s |
| Build Verilog | 1.427 s | 1.409 s |
| First measured full C++ run | 0.418 s | 0.328 s |
| First measured full Verilog run | 0.329 s | 0.278 s |
| Warm C++ median, one worker | 0.093077 s | 0.029155 s |
| Warm Verilog median | 0.006656 s | 0.006545 s |
| Warm native peak RSS median | 3.33 MiB | 2.53 MiB |

| Artifact size | Expanded expressions | Typed Table |
| --- | ---: | ---: |
| Final IR | 6,491,879 bytes | 3,175,505 bytes |
| Generated C++ total | 1,793,919 bytes | 1,274,638 bytes |
| Generated Verilog total | 319,319 bytes | 203,699 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUT behavior, arithmetic helpers, independent golden values, stimuli,
configuration, cycle limits and timeouts remain unchanged. The RTL harness now
drives the standard managed phases required by the nested state Rule.

```sh
pycircuit run examples/bf16_fmac --target cpp --cycles 86
pycircuit run examples/bf16_fmac --target verilog --cycles 86
```
