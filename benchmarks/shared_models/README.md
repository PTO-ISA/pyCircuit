# Bounded model evaluation

The installed command is `pycircuit benchmark PROJECT --output NEW_DIRECTORY`.
Use `pycircuit init PROJECT --template compute|stateful|backpressure` first.
These are existing single-worker models, not hierarchical projection examples.
The computation, recurrence and backpressure variants share exact 16-bit input
words, explicit protocol flags and independent expected packets/state.

Each benchmark first compiles/links/emits/builds both native execution views and
checks complete raw completion sequences and state. It repeats that pipeline
warm, then runs the same generated DUT in summary mode. Every completion in
every batch is compared exactly with the supplied golden raw packet; final
state is checked for every batch. No golden value is assigned to DUT outputs.
One warm-up invocation is excluded; default5 measurements at batch counts1,32,256
retain raw JSONL records, medians, IQR, min/max. Model initialization/reset occurs
outside each native-loop timer. Parsing occurs once per process; driving,
handshake, high/low samples, raw packet validation and fingerprint are inside.
Process wall includes all construction/reset and summary I/O. This is not a
bare-kernel or uninterrupted steady-state rate.

Functional rates count four completed enabled requests per batch (one invalid),
not six input records or three valid payloads. Timing reports successful rising cycles
per second and separately completed requests/results. Never form a speedup from
transactions/s divided by cycles/s. Warm/cold source/IR/emit timing and generated
C++ compile timing are distinct phases. Cold/warm build cost currently has one
observation per project; no compile-time median/variance claim. Peak memory is
GNU time's maximum process RSS, not the live sum of an entire process tree.
Artifact and native executable byte sizes are separate. Shared-host/unpinned CPU
noise and short-batch overhead must remain visible. Volume scaling is not
module-count/compiler scaling; those future results are NOT_RUN.

## External adapter

`pymtl_baseline.py` pins the actual installed PyMTL3 version3.1.17 and evaluates
a separate Normalize16 combinational kernel with exact independent values.
It supports Python and official Verilog translation/import modes:

```sh
python -m venv evaluation-venv
./evaluation-venv/bin/pip install pymtl3==3.1.17
# Run in an empty evaluation output directory, not in a source checkout.
./evaluation-venv/bin/python /path/to/pymtl_baseline.py --mode python --output python.json
./evaluation-venv/bin/python /path/to/pymtl_baseline.py --mode verilator --output native.json
```

The imported native mode additionally needs a working Verilator package lookup,
compiler, CFFI and its normal dependencies. Set PATH/VERILATOR_ROOT to the pinned
5.048 kit as in pyCircuit's setup action; do not mask diagnostics. Failed setup
is recorded as NOT_RUN with its error and no rate. Python and imported-native
modes are separate results. This kernel lacks pyCircuit project queues/state;
**neither mode is an equivalent whole-project performance comparison**. No
cross-tool ratio or ranking is computed. XLS and arcilator are research baselines
in the [product plan](../../docs/development/model-tool-product-plan.md), with
fixed source revisions; runtime adapters/equivalent workloads are still NOT_RUN.

Current SOTA status: **NOT_ESTABLISHED**. RTL numerical/time checks do not
establish synthesis, technology area/fmax, arbitrary projection or equivalence.
