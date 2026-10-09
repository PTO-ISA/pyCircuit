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

Module scope composes pure rules to compute each candidate from inputs or its
preceding stage's old Q. These calls describe combinational computations and add
no module instances or storage. `carry_save` serves six reduction sites;
`ripple8` serves all three carry-select chains; `right_barrel26` serves both
alignment paths and output normalization. The priority LZC and left barrel
remain explicit gate networks. Helpers do not call other rules or write owners.

One nine-argument `advance_fmac` rule snapshots old output Q and old S3 valid
before proposing all three stage records. S1, S2 and S3 advance
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
refactor. The existing `bf16_fmac` driver additionally runs 32 cases in each worker mode
covering partial X/Z data, unknown-enable failure, hold and reset recovery. The shared verifier also compares its `FRAME`/`FAILURE`/`CASES` records between
workers. This partial-X/Z coverage remains bounded; it is not an exhaustive
four-state or IEEE arithmetic claim.

## Build and inspect

```sh
cmake -S examples/bf16_fmac -B /absolute/build/bf16_fmac -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/bf16_fmac --parallel 2
ctest --test-dir /absolute/build/bf16_fmac --output-on-failure --no-tests=error
```

The source is 356 lines rather than 437, with nine writer arguments rather than
34. One CSA, ripple and right-barrel definition is reused six, three and three
times respectively. Final IR shrinks from 1,514,827 to 1,372,784 bytes and the
generated C++ header from 374,315 to 296,062 bytes. RTL increases from 71,993
to 76,088 bytes; smaller output is not claimed for every backend.

A recorded one-worker benchmark of 200,000 sampling epochs, after 4,096 warmup
epochs, has median 0.145491 seconds before and 0.0953922 seconds after across
five runs, with matching checksums. That is approximately 34.4% less time for
this workload, not a general simulator performance guarantee. Final installed
DUT code and wrappers are byte-identical to the measured refactor artifacts.
Compilation-speed or hardware area/timing improvements are not claimed.

## Generated system usage

`bench.py` exports `example_bf16_fmac.bench.ExerciseBF16Fmac`. The explicit source
closure is `bf16_fmac.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/bf16_fmac --target cpp --cycles 86` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

The complete 52-operand corpus and all original rising-edge valid/bubble
inputs run for 86 regular cycles, including four final drain observations.
Literal goldens use the retained independent highest-set-bit arithmetic oracle
and a three-token scoreboard, preserving all finite-bit anchors and result
holds. The original midstream reset/drop, held-level and four-state oracle
matrices remain separate; regular-clock reset inputs are data cycles here.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
