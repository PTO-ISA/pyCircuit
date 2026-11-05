# Standalone 16×16 array verification

`generate.py` is a data-only, exact rational arithmetic and protocol oracle. It
never imports the DUT. Run it to reproduce `cases.json`; then run
`python emit_stimulus.py` to refresh the literal input ROMs in `bench.py`.
The ROMs store the 14 distinct operand vectors once and retain all 235 scheduled
instructions. No expected acceptance flag or result is emitted into the bench.
The host oracle rounds the product before the
sum, applies RTZ/FTZ and finite saturation, and tracks only accepted operations.

`python test_oracle.py` checks hand-derived arithmetic anchors and protocol
accounting. `python run.py` compiles independent source units, links the generated
System, emits C++ and Verilog from the same final artifact, and executes C++ with
workers 1 and 2. Configure `PYC_TOOLCHAIN_ROOT` through `--toolchain` and use the
checkout's compiler/runtime environment. The runner never rebuilds or installs
the compiler. Evidence goes to a fresh ignored `docs/gates/logs/fa2-p0t-array/`
directory; no management logs are test inputs.

The schedule includes dense K=1/4/16/64; all 16 single-query row positions;
nonrectangular and empty cell masks; changing column masks; bubbles; delayed
take; invalid lengths, tags and indices; early end; ACTIVE/RESULT abort; and
restart. Every result checks all 256 words, tag, accepted step count and useful
MAC count. A final nonfused/FTZ case distinguishes product rounding from FMA.

The transcript contract is `array_action(cycle, op, tag, index)`,
`array_row(tag, row, c0, ..., c15)`, `array_result(tag, steps, useful_macs)`, and
one `array_pass(total_results)`. Only even `commit_epoch` records correspond to
successful rising edges. Duplicate, missing, reordered or nonaccepted actions
fail; every result row must occur at its accepted take epoch. Worker traces
must agree exactly. Latency, stalls and step intervals derive from accepted
cycles; the simulation horizon is solely a completion bound.

Delayed take tests RESULT holding, not Queue resource backpressure. Queue
atomic take+push, midoperation reset, X/Z fault injection and nonfinite operands
remain explicitly NOT RUN in this first schedule. Verilog simulation, complete
attention, DMA/softmax integration and physical frequency are outside this test.
A host-oracle pass is not generated hardware execution evidence. See
[the current stage results](../../VALIDATION.md) before interpreting a run.
