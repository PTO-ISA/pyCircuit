# Two-stage packet pipeline

[Generated MLIR, C++ and RTL examples](GENERATED.md)

This example implements the `features/pipeline_builder` hardware with its
fixed 32-bit word width. Two `PipelinePacket` registers each store
`word: ac.u32` and `valid: ac.u1`. Stage 0 captures the scalar inputs. At the
same transaction, stage 1 captures old stage 0 with its word incremented modulo
2^32. The typed result reports old stage 1, so there is no combinational bypass
from either the input or stage 0 to the output.

Both stages have recursively zero reset images. Work observes old register
values for the entire transaction, and Xfer commits both proposals together.
Thus the first samples after reset report `word=0, valid=0`; a valid input needs
one capture edge into stage 0 and another edge into stage 1 before a following
Work reports its incremented word. Unknown word bits participate in the existing
fixed-width addition rather than being replaced by a host calculation.

`PipelineBuilder` directly calls the same-source state-owning `PipelineStorage`
module. The stateless root inherits the generated sampling and reset pins. The
Python source contains no clock, reset, DFFE or current/next API. Its public
inputs are `word` and `valid`, and its single physical `result` value is the
ordered nominal `PipelinePacket` struct.

Build this design through the shared example helper and an installed pyCircuit
package:

```sh
cmake -S examples/pipeline_builder -B /absolute/build/pipeline-builder -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/pipeline-builder --parallel 4
ctest --test-dir /absolute/build/pipeline-builder --output-on-failure --no-tests=error
```

Acceptance requires the generated C++ model with one and two workers and the
RTL model to agree with independent latency, hold and wraparound oracles. Source
presence alone is not execution evidence.

## Generated system usage

`bench.py` exports `example_pipeline_builder.bench.ExercisePipelineBuilder`. The explicit source
closure is `pipeline_builder.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/pipeline_builder --target cpp --cycles 72` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

This bench runs all 71 original rising-edge data/valid inputs plus a
final observation cycle. Literal expectations are derived from the retained
independent C++ scoreboard along the regular-clock, resetless trajectory; they
never drive DUT results. Original reset edges are ordinary data cycles here.
All original deterministic traffic and arithmetic boundaries remain represented.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
