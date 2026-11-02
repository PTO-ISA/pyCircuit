# One source, functional and timing execution

The source declares a worker's calculation and state/effects once, plus finite
queues and II. The native linker automatically derives a functional or timing
entry from that declaration; C++ and Verilog are independent backend choices.
There is no second author-written functional DUT in the active build.

`timed_word_pipeline.py` imports the existing public WordArithmetic provider
from `../functional_float/word_arithmetic.py`. `WordWorker` owns accumulation
and completion/rejection state. The wrapper owns admission and timing.
`alternate_model.py` supplies a different 16-bit state-dependent XOR/add worker,
with different method, field and instance names.

| Model | Ingress depth/latency | Egress depth/latency | II |
| --- | --- | --- | --- |
| `TimedWordPipeline` | 2 / 1 | 2 / 3 | 1 |
| `TimedWordPipelineAlt` | 1 / 1 | 1 / 1 | 2 |
| `TimedDigestPipeline` | 1 / 2 | 2 / 1 | 1 |

`@ac.module(execution={...})` names the roles checked by the compiler. Its
functional projection removes only proved FIFO waiting and the private II gap.
Both strategies invoke the same generated worker and preserve request order,
accepted-input snapshots and exactly-once state effects. Timing retains queue
capacity, maturity and backpressure. Functional execution uses a serial,
always-ready response sink; it does not simulate empty wait cycles.

## Run with an installed CompilerDev and Runtime

From the repository root, set `PYC_TOOLCHAIN_ROOT` to an installation built from
this candidate. Commands use only public source and installed tools:

```sh
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
pycircuit run examples/timed_word_pipeline \
  --execution functional \
  --model timed_word_pipeline.timed_word_pipeline.TimedWordPipeline \
  --inputs examples/timed_word_pipeline/requests.json --target cpp \
  --build-dir .pycircuit_out/word-functional-cpp
pycircuit run examples/timed_word_pipeline \
  --execution timing \
  --model timed_word_pipeline.timed_word_pipeline.TimedWordPipeline \
  --inputs examples/timed_word_pipeline/requests.json --target verilog \
  --cycles 24 --consumer-start 8 \
  --build-dir .pycircuit_out/word-timing-rtl
```

Use `--target cpp|verilog` with **either** execution strategy. For the alternate
profile select `timed_word_pipeline.timed_word_pipeline.TimedWordPipelineAlt`.
For Digest select `timed_word_pipeline.alternate_model.TimedDigestPipeline`,
`digest_requests.json` and timing `--consumer-start 10`. Use separate build
directories for each model, strategy and backend. C++ supports `--workers 1|2`;
RTL input execution uses Icarus (`iverilog` and `vvp`).

Each input row has `active` and a raw `request` record. Integer fields must fit
their declared Bits widths; exact binary strings also support X/Z. The adapter
holds unacknowledged offers. `model-trace.jsonl` records availability and raw
bits (unavailable is null); `run-execution.json` records execution status and
strategy. Execution success alone is not numerical/timing verification.

For direct source-unit use, compile the provider and the model independently,
then link the **same** unit closure in either strategy:

```sh
pycircuit compile -c examples/functional_float/word_arithmetic.py \
  --source-root examples/functional_float --package-prefix functional_float \
  -o .pycircuit_out/units/word
pycircuit compile -c examples/timed_word_pipeline/timed_word_pipeline.py \
  --source-root examples/timed_word_pipeline --package-prefix timed_word_pipeline \
  -I .pycircuit_out/units/word -o .pycircuit_out/units/pipeline
pycircuit link .pycircuit_out/units/word .pycircuit_out/units/pipeline \
  --top timed_word_pipeline.timed_word_pipeline.TimedWordPipeline \
  --execution functional -o .pycircuit_out/word-functional.ac
pycircuit link .pycircuit_out/units/word .pycircuit_out/units/pipeline \
  --top timed_word_pipeline.timed_word_pipeline.TimedWordPipeline \
  --execution timing -o .pycircuit_out/word-timing.ac
pycircuit emit .pycircuit_out/word-functional.ac --target cpp \
  -o .pycircuit_out/word-functional
pycircuit emit .pycircuit_out/word-functional.ac --target verilog \
  -o .pycircuit_out/word-functional-rtl
```

These roots and their submit/state wiring are compiler-generated. The CMake
helper exposes `PROVIDER_UNITS` for the provider closure and `MODEL_SOURCES`
for the component closure, separate from its consuming System bench.

## Independent checks

```sh
python -m pytest -q tests/system/test_shared_execution_models.py
```

This checks functional/timing × C++ workers 1/2/Icarus for all three profiles,
using independent raw/flag/state and cycle-level FIFO expectations. Word's
eight logical offers include one disabled offer, seven accepted architectural
completions (including one invalid), and **six valid publications**: tags
0, 1, 4, 5, 6, 7 with values 3, 12, 11, 15, 16, 8. Final accepted/total/completed/
rejected is 7/65/7/1. Digest has five offers, four accepted completions, one
invalid and three valid values 7, 10, 18; final state is 4/18/4/1.

Invalid completion flags are visible immediately in the functional entry but
never authorize a valid payload. The timing result FIFO contains valid
responses only. Disabled offers acknowledge the offer without acceptance or
worker effects. The tests compare valid publication order and final state,
not identical per-cycle waves between execution strategies.

The normal authored timing System also remains runnable through the existing
example verifier:

```sh
cmake -S examples/timed_word_pipeline -B .pycircuit_out/word-system -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PYC_TOOLCHAIN_ROOT"
cmake --build .pycircuit_out/word-system --parallel 2
ctest --test-dir .pycircuit_out/word-system --output-on-failure --no-tests=error
```

That System runs 24 cycles/48 epochs. The independent model-entry tests, rather
than this System's final assertions alone, establish latency/II/capacity and
backpressure recovery. The functional entry is generated by the native projection pass; no separately
authored functional DUT is part of this example.

Projection rejects architectural timers/visible wait state, cancellation,
multiple workers/arbiters/domains, unsupported resources or a trapping worker.
Active unknown data/control fails before new effects; inactive or disabled
unknown payload has no effect. See the
[execution contract](../../docs/development/shared-functional-timing-model.md)
for the exact admitted slice and unverified boundaries.
