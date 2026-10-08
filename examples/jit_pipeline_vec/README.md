# Three-stage tagged pipeline

[Generated MLIR, C++ and RTL](GENERATED.md)

`JitPipelineVec(a: ac.u16, b: ac.u16, sel: ac.u1)` computes a comparison tag
and either a 16-bit modular sum (`sel=1`) or a bitwise XOR (`sel=0`). Three
zero-initialized `Packet` variables carry the tag and data. The typed result
contains `tag: ac.u1`, `data: ac.u16` and its low byte, `lo8: ac.u8`.

The rule reads the third packet for its output before shifting the three packet
values. All state updates commit together. A packet appears in a Work sample
after its third successful capture edge; the low-byte slice adds no register.
Reset clears all three packets. Repeated clock levels hold state. The Python
source uses ordinary state, a rule and a direct module call; the runner owns
the generated physical sampling/reset inputs.

```sh
cmake -S examples/jit_pipeline_vec -B /absolute/build/jit-pipeline-vec -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/jit-pipeline-vec --parallel 4
ctest --test-dir /absolute/build/jit-pipeline-vec --output-on-failure --no-tests=error
```

The gate runs generated native models with one and two workers and compares
Work samples against RTL. Native discard/retry and macro-enabled Icarus checks
cover the additional transactional and four-state behavior.

## Generated system usage

`bench.py` exports `example_jit_pipeline_vec.bench.ExerciseJitPipelineVec`. The explicit source
closure is `jit_pipeline_vec.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/jit_pipeline_vec --target cpp --cycles 75` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

This bench runs all 74 original rising-edge data/valid inputs plus a
final observation cycle. Literal expectations are derived from the retained
independent C++ scoreboard along the regular-clock, resetless trajectory; they
never drive DUT results. Original reset edges are ordinary data cycles here.
All original deterministic traffic and arithmetic boundaries remain represented.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
