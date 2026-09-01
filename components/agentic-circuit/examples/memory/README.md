# Agentic Circuit memory examples

## Parameterized high-level blocks

`davincioo_jit.py` keeps every dynamic connection as a typed Queue SSA edge,
freezes `CoreConfig` through `ac.jit`, and uses the simple high-level block
names `compute`, `route`, `pipeline`, `merge`, `schedule`, and `reorder`.
Only `compute` carries a lambda; other blocks select typed payload fields with
compile-time descriptors and bind optimized gfsim and PYC/Verilog providers.

```bash
PYTHONPATH=src python - <<'PY'
from examples.architecture.davincioo_jit import specialization

open("build/davincioo.ac.mlir", "w").write(specialization.lower_acir())
open("build/davincioo.cpp", "w").write(specialization.lower_cpp())
PY

c++ -std=c++20 -I include -fsyntax-only build/davincioo.cpp
```

`specialization.materialize_cpp(cache_root)` compiles the generated C++ on
first use and returns the content-addressed cached object later.
`materialize_pyc(...)` publishes the corresponding PYC/C++/Verilog bundle.

`multirate_compute.py` freezes `rate=4` into the Queue and C++ template
identities. Native QueueGraph/gfsim supports it; PYC deliberately rejects
`rate>1` until shared ordered FIFO lane lowering is implemented.

### DavinciOO canonical PTO trace

The trace runner converts the locked DavinciOO JSONL trace to canonical PTO
trace JSON, specializes the high-level Python model, compiles generated gfsim,
and checks completion order, retirement order, architectural values, and the
453-cycle reference contract. It also emits a deterministic instruction
swimlane.

```bash
.venv/bin/python tools/run-davincioo.py
```

Checked-in evidence:

- [`davincioo-softmax-run.json`](../../tests/goldens/davincioo/davincioo-softmax-run.json)
- [`davincioo-softmax-swimlane.svg`](../../tests/goldens/davincioo/davincioo-softmax-swimlane.svg)

## Explicit memory (epoch 0.4)

`memory_simple.py` declares one root-owned, 16-entry `u16` memory and connects
two typed logical endpoints from child scopes. Writer endpoint ordinal 0 has
fixed priority over reader endpoint ordinal 1.

The harness makes both endpoints valid together. Two writes to address `3`
return old values `0` and `42`; only after those responses complete can the
reader run, and it observes the final value `99`. This exercises:

- one physical memory shared by multiple logical request endpoints;
- root-to-descendant scope visibility;
- canonical fixed-priority arbitration;
- one outstanding request and global busy backpressure;
- endpoint-specific write policies and response demultiplexing;
- old-data write responses.

From the repository root:

```bash
PYTHONPATH=src \
  .venv/bin/python \
  tools/ac-queue-cxxgen.py examples/memory/memory_simple.py \
  --system memory_simple \
  --acir-output /tmp/memory_simple.mlir \
  --plan-output /tmp/memory_simple.plan.json \
  --acir-opt build/dev-llvm22/bin/acir-opt \
  --queue-plan-tool build/dev-llvm22/bin/acir-queue-plan \
  --queue-cxxgen-tool build/dev-llvm22/bin/acir-queue-cxxgen \
  --output examples/memory/memory_simple.generated.cpp

c++ \
  -std=c++20 -Iinclude -Iexamples/memory \
  examples/memory/memory_simple_harness.cpp \
  -o /tmp/memory_simple_sim

/tmp/memory_simple_sim
```

Expected output:

```text
cycles=8 write_old_values=0,42 read_after_priority=99
```

The generated `memory_simple.generated.cpp` is a build artifact and is not
checked in.

## Statically expanded memory banks

`memory_banks.py` uses a homogeneous `ac.array` of four memories. The
Python-only `banks.select(...).request(...)` expansion makes bank selection
explicit while lowering entirely to the existing `ac.route`, four
`ac.memory.instance`/`ac.memory.request` pairs, and one `ac.merge`. Each bank
has independent storage and one outstanding request; responses from different
banks may be reordered, so the harness checks them by tag.

```bash
PYTHONPATH=src \
  .venv/bin/python \
  tools/ac-queue-cxxgen.py examples/memory/memory_banks.py \
  --system memory_banks \
  --acir-output /tmp/memory_banks.mlir \
  --plan-output /tmp/memory_banks.plan.json \
  --acir-opt build/dev-llvm22/bin/acir-opt \
  --queue-plan-tool build/dev-llvm22/bin/acir-queue-plan \
  --queue-cxxgen-tool build/dev-llvm22/bin/acir-queue-cxxgen \
  --output examples/memory/memory_banks.generated.cpp

c++ \
  -std=c++20 -Iinclude -Iexamples/memory \
  examples/memory/memory_banks_harness.cpp \
  -o /tmp/memory_banks_sim

/tmp/memory_banks_sim
```

Expected output has the following values; the cycle count is deterministic but
is intentionally not part of the example contract:

```text
cycles=<N> bank0=41 bank1=91 bank2_initial=0
```

## Single-memory busy backpressure

`memory_busy.py` uses `ac.memory(..., latency=3)` and issues two reads at
different simulated times. Its harness prints the public request Queue
occupancy after every epoch: the second request remains queued throughout the
physical access latency, then is accepted only after the first response
releases `busy`.

```bash
PYTHONPATH=src \
  .venv/bin/python \
  tools/ac-queue-cxxgen.py examples/memory/memory_busy.py \
  --system memory_busy \
  --acir-output /tmp/memory_busy.mlir \
  --plan-output /tmp/memory_busy.plan.json \
  --acir-opt build/dev-llvm22/bin/acir-opt \
  --queue-plan-tool build/dev-llvm22/bin/acir-queue-plan \
  --queue-cxxgen-tool build/dev-llvm22/bin/acir-queue-cxxgen \
  --output examples/memory/memory_busy.generated.cpp

c++ \
  -std=c++20 -Iinclude -Iexamples/memory \
  examples/memory/memory_busy_harness.cpp \
  -o /tmp/memory_busy_sim

/tmp/memory_busy_sim
```

Expected output:

```text
epoch  req_q  received  event
    0      1         0  request A queued
    1      1         0  A accepted; request B queued
    2      1         0  B blocked by memory latency
    3      1         0  B blocked by memory latency
    4      1         0  A response accepted; busy released
    5      0         1  B accepted
    6      0         1  B response pending
    7      0         1  B response pending
    8      0         1  B response accepted; busy released
    9      0         2  sink received B
latency_blocked=1 accepted_after_release=1 responses=2
```

## DMA-style DRAM-to-SRAM copy

`dma.py` declares root-owned DRAM and SRAM instances. Inside
`with ac.scope("dma")`, the DRAM response Queue directly drives an SRAM write
endpoint. The harness first seeds `DRAM[5]` with `0x1234`, runs one DMA copy to
`SRAM[3]`, then reads SRAM back to verify the transferred value.

```bash
PYTHONPATH=src \
  .venv/bin/python \
  tools/ac-queue-cxxgen.py examples/memory/dma.py \
  --system dma \
  --acir-output /tmp/dma.mlir \
  --plan-output /tmp/dma.plan.json \
  --acir-opt build/dev-llvm22/bin/acir-opt \
  --queue-plan-tool build/dev-llvm22/bin/acir-queue-plan \
  --queue-cxxgen-tool build/dev-llvm22/bin/acir-queue-cxxgen \
  --output examples/memory/dma.generated.cpp

c++ \
  -std=c++20 -Iinclude -Iexamples/memory \
  examples/memory/dma_harness.cpp \
  -o /tmp/dma_sim

/tmp/dma_sim
```

Expected output:

```text
seed_tick=5 copy_tick=14 verify_tick=19 dram_value=0x1234 copy_old_sram=0
```

## Current boundary

The 0.4 contract intentionally supports one physical port and one outstanding
request per memory instance. It does not provide true multi-port access,
round-robin fairness, response reordering, byte enables, or non-zero
initialization. A continuously valid higher-priority endpoint can starve lower
ordinals. All endpoints of an instance must use the same payload struct and
the memory data/address widths are limited to 64 bits.
