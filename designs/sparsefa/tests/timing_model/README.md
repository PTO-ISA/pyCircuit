# SparseFA streamed-Index C++ timing model

`shared_kv_trace.cpp` is an **independent, test-owned cycle model** of the
draft Shared-KV SparseAttention architecture. It advances Count/Q transfer,
two 64-entry Index slots, ordered per-batch validation budgets, KV request
credits and shared response bus, independent KV slots, four 16-head QK / softmax
/ PV waves per batch, and O write acknowledgement. It does not execute a
pyCircuit-generated System, RTL, a real DMA, or numerical attention. The
[streamed Index NDF](../../ndf/l2/fa2-core-idx-stream-1007.md) defines the
architecture; the cycle budgets below belong only to this performance model.

The model first loads Count and Q, then loads Index batch 0. Once its 64-entry
validation budget completes, batch 0 immediately reserves a KV slot and issues
KV row requests. Index batch 1 loads into the other Index slot while those
requests and responses proceed. Later Index batches wait for a released slot.
An Index slot stays occupied until all 64 requests for its batch have issued;
KV metadata is assumed to transfer into its KV slot before release. The two
Index slots are present in both runs; the single/two-slot comparison changes
**only the number of KV slots**. Index and KV responses use one 32 B/cycle
nonpreemptive bus. The renderer checks bus exclusion, maximum eight KV row
credits, slot reuse, ordered validation/computation, all row requests and
completions, and that the first KV request precedes validation of the full
Index list.

The input is assumed to contain precomputed, valid, pairwise distinct token
IDs, with no causal masking and `Sk >= Count`. The model assigns 64 cycles per
Index batch to an assumed validation scheduler but **does not read real IDs,
perform bounds/Patricia duplicate checks, or model error/drain paths**. Thus
these runs are successful-path performance estimates, not semantic validation
of the Index hardware.

## Model parameters

| Parameter | Assumption |
| --- | --- |
| Heads, head dimension, batch | 64, 64, 64 Index entries |
| Count range | Multiple of 64 from 64 to 2048 |
| Index slot capacity | Two independent 64 × uint32 slots |
| KV slots | One or two, compared with otherwise identical assumptions |
| External response bus | 32 B/cycle, one response stream |
| Index startup and 256 B response | 12-cycle startup, then 8 bus cycles |
| KV row | 16-cycle response latency, four 32-byte beats for D=64 FP16/BF16 |
| KV outstanding credits | 8 |
| QK and PV | 44 cycles per 16-head wave each; four 16-column tiles × 11-cycle array cadence |
| Softmax | 97 cycles per 16-head wave, explained below |
| State commit | One cycle per wave |
| Output write | 8192 B / 32 B/cycle + 12-cycle acknowledgement |

The softmax budget is a **serial stage estimate** based on the
[Vector resource contract](../../ndf/l2/fa2-core-vector-1006.md). A full wave
contains 16 × 64 = 1024 score/weight elements. Four ordinary FP32 passes
(max/compare, shifted score, row reduction, state update) each take
`ceil(1024/64) = 16` issue cycles at 64 elements/cycle. The shared exp unit
handles 1024 weight exponentials and 16 alpha exponentials at 32/cycle, taking
`ceil(1040/32) = 33` issue cycles. The sum is **97 cycles/wave**. This is a
simplified resource schedule, not a proven implementation latency; pipeline
fill, scratch-port conflicts, dependency stalls and actual numerical results
are outside this model. QK/PV timing is also a model assumption.

## Reproduce the executed traces and PNGs

Run from this worktree's repository root with its own C++20 compiler. The
output prefix is arbitrary; the paths below recreate the asset snapshots.

```sh
mkdir -p .pycircuit_out
/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-g++ \
  -std=c++20 -O2 -Wall -Wextra -Werror \
  designs/sparsefa/tests/timing_model/shared_kv_trace.cpp \
  -o .pycircuit_out/shared_kv_trace
.pycircuit_out/shared_kv_trace .pycircuit_out/sparsefa-index-stream-128 128
python3 designs/sparsefa/tests/timing_model/render_trace.py \
  .pycircuit_out/sparsefa-index-stream-128
.pycircuit_out/shared_kv_trace .pycircuit_out/sparsefa-index-stream-2048 2048
python3 designs/sparsefa/tests/timing_model/render_long_trace.py \
  .pycircuit_out/sparsefa-index-stream-2048
```

Both C++ runs and both renderers exited successfully. The renderers consume
the actual CSV events, run the checks described above, and emit SVG/PNG; they
do not draw a hand-authored schedule.

| Count | One KV slot | Two KV slots | Saved by second KV slot |
| ---: | ---: | ---: | ---: |
| 128 | 2674 cycles | **2402 cycles** | 272 cycles (10.2%) |
| 2048 | 33394 cycles | **24730 cycles** | 8664 cycles (25.9%) |

For both runs, Count ends at cycle 13 and Q preloading ends at cycle 285.
Index batch 0 completes its validation budget at cycle **369**; the first KV
row request and Index batch 1 DMA start at that same cycle. Thus the first KV
request does not wait for the remaining Index entries. In the 2048-item run,
Index batch 2 starts at cycle 614 after slot 0 is released. All **2048** KV
row requests and completions appear in the 2048-item trace; peak outstanding
KV rows is **8**. The two-slot run finishes at cycle 24730, including O write
acknowledgement.

Review the executed [128-Index PNG](assets/sparsefa-index-stream-128-overview.png),
[2048-Index full-run PNG](assets/sparsefa-index-stream-2048-overview.png), or
[2048-Index startup/detail PNG](assets/sparsefa-index-stream-2048-detail.png).
The corresponding SVG files and one/two-slot CSV traces are in `assets/` under
the same prefixes. Earlier `shared-kv-*` assets retain their **historical**
all-Index-first, 16-cycle-softmax schedule and must not be used as results for
the streamed-Index model.

Generated-hardware C++/RTL simulation and PPA are **NOT RUN** for this draft
Shared-KV target. Previous P0S generated-System attempts are recorded under
ignored `docs/gates/logs/fa2-p0-completion/`; they did not produce a runnable
Shared-KV System or a DUT trace.
