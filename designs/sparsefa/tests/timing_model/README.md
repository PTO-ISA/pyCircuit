# Shared-KV C++ timing-model trace

`shared_kv_trace.cpp` is a **test-owned executable cycle model** of the draft
Shared-KV scheduler. It is not generated from the pyCircuit hardware source and
does not compute attention values. Its CSV contains events produced while the
model advances Count/Index/Q transfer time, bounded DMA credits, row responses,
independent KV slots, head waves, QK/softmax/PV stages, write acknowledgement
and completion cycle by cycle. `render_trace.py` creates the SVG/PNG from those
events and verifies their timing order, complete KV traffic and overlapping
slot use. The model assumes precomputed, valid, distinct Index IDs that
are not masked by causal rules. It budgets one cycle per ID for validation but
does **not** ingest or check real Index contents, issue address-bearing memory
transactions, or compute attention values.

Default scenario: H=64, D=64, Count=128 with two 64-item Index batches, 8
outstanding KV row credits, 32 B/cycle bus, 12-cycle Index startup, 16-cycle KV
row latency, 5-cycle array drain and 16-cycle softmax stage per head wave.
Every 16-column array tile has its own begin/GROUP/END/drain/take lifecycle.
Count is 4 B and takes 13 cycles including startup; Q is 8192 B and takes 272
cycles including memory latency. Count, Index, Q, KV responses and output
writeback share the modeled 32 B/cycle bus, with Count/Index/Q loaded in order
before the first KV request.
This parameter set is a model assumption, **not measured hardware latency**.
The optional `INDEX_COUNT` argument accepts a multiple of 64 from 64 to 2048;
omitting it preserves the 128-item default.

From the repository root, using a C++20 compiler:

```sh
mkdir -p .pycircuit_out
/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-g++ \
  -std=c++20 -O2 -Wall -Wextra -Werror \
  designs/sparsefa/tests/timing_model/shared_kv_trace.cpp \
  -o .pycircuit_out/shared_kv_trace
.pycircuit_out/shared_kv_trace .pycircuit_out/sparsefa-shared-kv
python3 designs/sparsefa/tests/timing_model/render_trace.py \
  .pycircuit_out/sparsefa-shared-kv
```

The executed default run produced 2170 cycles with one slot and 1898 cycles
with two slots, saving 272 model cycles (12.5%). In the two-slot CSV, batch 0
computes during cycles 725–1177 while slot 1 fills during cycles 725–997. The
raw CSV records 128 accepted KV row requests and completions, and the model's
peak outstanding count is 8. Both slot counts use the same inputs and memory
timing assumptions. The raw CSV and rendered evidence are generated under
the requested output prefix and can be regenerated. Snapshots of this exact
run are included for review: [PNG](assets/shared-kv-2slot-model.png),
[SVG](assets/shared-kv-2slot-model.svg),
[single-slot CSV](assets/shared-kv-1slot-model.csv) and
[two-slot CSV](assets/shared-kv-2slot-model.csv). Tests do not read the
snapshots.

For the 2048-item long sequence, use the same executable with a separate
output prefix and run the long-trace renderer:

```sh
.pycircuit_out/shared_kv_trace .pycircuit_out/sparsefa-shared-kv-2048 2048
python3 designs/sparsefa/tests/timing_model/render_long_trace.py \
  .pycircuit_out/sparsefa-shared-kv-2048
```

The executed 2048-item run produced **32 batches** and 2048 KV row requests.
It assumes `Sk >= 2048` so all selected token IDs can be valid.
One slot took **26,410 model cycles**; two slots took **17,978 cycles**, a
difference of 8,432 cycles (31.9%) under the same timing assumptions. The
complete Index prefix reaches its timing barrier at cycle 2701, Q finishes
loading at 2973, batch 0 fills during 2973–3245 and computes during
3245–3697, while slot 1 fills batch 1 during 3245–3517. The 31 later batch
fills overlap computation in the other slot. Read the
[full-run PNG](assets/shared-kv-2048-overview.png) or
[startup and first five compute batches](assets/shared-kv-2048-detail.png),
with zoomable [full-run SVG](assets/shared-kv-2048-overview.svg) and
[detail SVG](assets/shared-kv-2048-detail.svg). The corresponding
[one-slot CSV.gz](assets/shared-kv-2048-1slot.csv.gz) and
[two-slot CSV.gz](assets/shared-kv-2048-2slot.csv.gz) preserve the executed
events in gzip form; decompress them to compare with a fresh run.
No actual Index IDs or attention values were evaluated.

The generated pyCircuit P0S System was also attempted from this checkout. Its
source units compiled after a temporary local Rule-syntax migration, but native
link hit
the compiler's default analysis work budget in `fa2_scheduler.py`; directly
linking `FA2Engine` hit the same budget in `fa2_engine.py`. Those failed
attempts are under ignored `docs/gates/logs/fa2-p0-completion/`. The trial
migration was discarded because it did not produce a runnable System. These
attempts do not
provide a generated-hardware trace, and this timing model must not be used as
one. The draft Shared-KV command, DMA, slots and integrated feeder remain
unimplemented; hardware C++/RTL simulation and PPA are NOT RUN.
