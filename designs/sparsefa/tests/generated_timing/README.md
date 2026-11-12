# pyCircuit-generated Shared-KV timing schedule

The [source](../../src/fa2_shared_kv_timing.py) describes a bounded, stateful
Shared-KV resource schedule in pyCircuit. The [System bench](bench.py) observes
that source. [run.py](run.py) publishes the two source units, links each System,
emits C++ and Verilog from the same verified final artifact, builds and runs the
generated C++ with workers 1 and 2, checks resource bounds, and writes event
CSV snapshots. [render.py](render.py) draws SVG swimlanes from those generated
events; its SVGs can also be converted to PNG. The existing
[handwritten C++ model](../timing_model/README.md) remains an independent
comparison, not an input to this generated path.

The four runs fix H=64, D=64, 64 Index entries per batch, two Index slots,
and one or two KV slots. An Index value is assumed precomputed and valid.
The source schedules an Index fetch and validation budget, eight outstanding
KV row requests at a time, one shared K=V row per token, and QK/softmax/PV
resource budgets. It does not fetch actual Index IDs or KV payloads, check
duplicate/causal/error cases, calculate attention values, execute the physical
4096-multiplier array, or model SRAM bank conflicts. These are generated
**abstract resource-model cycles**, not complete accelerator latency or PPA.
The separate 16×16×16 array System still exceeds the default link analysis
budget and has no generated execution result.

| Count | One KV slot | Two KV slots | Saved by second slot |
| ---: | ---: | ---: | ---: |
| 128 | 3021 cycles | 2572 cycles | 449 cycles (14.9%) |
| 2048 | 38901 cycles | 24837 cycles | 14064 cycles (36.2%) |

Workers 1 and 2 produced identical events in all four scenarios. The complete
run manifest, command exits and raw generated C++ logs are under the ignored
`docs/gates/logs/sparsefa-pyc-timing-20261111T100610.883626Z/`; the
source/bench/runner/toolchain hashes are recorded there. Verilog was emitted
from each final artifact but was not simulated, per the requested scope.

The [128-item diagram](assets/sparsefa-pyc-128-overview.svg),
[2048-item full run](assets/sparsefa-pyc-2048-overview.svg), and
[2048-item startup detail](assets/sparsefa-pyc-2048-startup-detail.svg)
come from the checked-in CSVs. The matching PNGs are in the same directory.
The Index lane shows allocation until its last KV row request; cyan KV
transfer ends when a batch fills, while pale cyan shows the filled slot held
through compute retirement. QK, Softmax and PV lanes are stage transitions
reported by the generated System.

Reproduce from this checkout with its installed CompilerDev toolchain and a
C/C++ compiler. `run.py` accepts `PYC_TOOLCHAIN_ROOT`, `CC` and `CXX`.

```sh
python3 designs/sparsefa/tests/generated_timing/run.py --snapshot
python3 designs/sparsefa/tests/generated_timing/render.py
```

To regenerate the optional PNG copies, use a converter such as `cairosvg` on
each SVG. The standalone Python renderer does not need a plotting package.
