# Index-prefix performance adapter

`driver.cpp` drives the generated `FA2TensorScheduler` product through public
ports. It never accesses generated private state or supplies expected DUT results.
Arguments are count (0..2048), response latency in rising edges, case name, and
worker count. Cases are `valid`, `duplicate`, `range`, `memory`, and `cancel`.

The input prefix is reverse ordered. Every external index request must visit its
original consecutive address; K/V reads must retain the supplied token order.
Q and K are zero, V is FP16 one, dimensions is one and scale is one. Thus all
scores are zero, all exponential weights are exactly one, and the final output
must be FP16 one for a nonempty prefix (zero for an empty prefix). This oracle is independent of product implementation.

No Q/K/V access is permitted before all indices have been read and validated.
Duplicate/range/error cases alter the final index response and require codes
4/3/5 without tensor access. Cancellation arrives while the second index request
is outstanding and requires draining it before code 6. Latency 1 uses ready on
every edge; delayed responses additionally stall request acceptance every fifth
edge. Only one request can be outstanding.

Metrics count rising-edge intervals from command acceptance to first accepted Q,
first accepted K/V, and visible completion. The JSON also reports host elapsed
milliseconds, which are simulation execution cost and not hardware latency.
Use the identical adapter and arguments against baseline and candidate generated
products. N=128,512,2048 valid cases establish scaling; both response schedules
and all fault cases establish bounded protocol checks. Existing integration
Systems remain responsible for broader layout, reset, numerical and recovery
coverage.

Build with C++20, the emitted product include directory, the matching runtime
include directory, `product-build/libpycircuit_modules.a`, and the same runtime's
`lib/libpyc6_runtime.a`, with `-pthread`. The adapter does not read management
logs or evidence receipts.

`run.py --product PATH --toolchain PATH --cxx CXX` builds the adapter against an
existing emitted product and runs workers 1 and 2 by default. `--workers 1` and
`--small-only` allow bounded selections. It records exact input hashes, commands,
partial failures, and result JSON under ignored gate evidence; tests themselves
never read those records. The prefix files are generated from `cases.py` into
the run output directory. The default matrix includes all short adversarial
prefixes and reverse-order N512/N2048; ascending/permuted larger prefixes remain
available in the catalog but are not part of the default matrix.

The optional fifth adapter argument is a whitespace-separated input file:
`key_length` followed by exactly N uint32 tokens. This permits high-bit divergence,
full unsigned-range cases, shared low bits, long shared prefixes, and all 32
branching bit positions without allocating a bitmap in the memory adapter.
K and V use separate large virtual address regions. `duplicate-first` repeats the
first token at position one and requires termination before any later index read.
`memory-range` returns an out-of-range final token with a memory error and requires
the memory error to win.

The optional sixth argument is a cancellation offset from the accepted response
for the second index. Offsets 5/6/7/8 cover the reviewed candidate's leaf write,
branch write, parent/root publication, and ordered-index publication boundaries.
Each cancellation case must drain outstanding work, publish code 6, then accept a
new command in the same DUT and complete the full valid prefix correctly. The old
implementation may be in a different internal phase at those same external
stimulus offsets; only its cancellation contract is compared. The adapter checks
request payload stability while acceptance is stalled.

These D=1 measurements isolate index validation cost. They are distinct from
D=64 numerical and causal-order checks in the existing integration Systems.

`trie_write_boundaries.json` is an independent address/packed-data oracle for
the shared node-write implementation. Its 14 entries are static review data,
not an executed generated-DUT test. The full N2048 public-port run exercises
the last legal allocation; direct injection of invalid private allocator state
has not been executed. Current measured results and integration failures are
reported in [VALIDATION](../../VALIDATION.md).

## Measurement and matching

For each logical cycle the adapter drives clock low, performs a successful
simulation step, and samples the public outputs. The sampled ready/valid pairs
identify the transfers on the following rising edge. It then drives clock high
with the same command, memory response and cancellation values and commits that
edge. A request accepted at edge R cannot receive a response before edge R+1.
A delay of four means the response first becomes valid at R+4; it remains valid
until response-ready accepts it. The delayed schedule deasserts request-ready
when the absolute logical cycle modulo five equals two. Responses and requests
are counted only on their respective ready/valid handshakes.

`first_q_latency` and `first_kv_latency` are the accepted read-request edge minus
the accepted command edge. `done_latency` is the completion acceptance edge minus
the accepted command edge; completion-ready is continuously asserted. A missing
read class is reported as -1. For cancellation cases `cancel_latency` measures
the cancelled command's completion. The other latency fields describe the
successful restarted command, with its own `accept_edge`. Cancellation offsets
are measured from response acceptance, not request acceptance or host elapsed
time. `host_ms` is informational wall time and must never substitute for these
hardware-cycle metrics.

The same immutable `driver.cpp`, `cases.py`, prefix bytes and arguments must be
used for old/new product comparisons. Each run records SHA256 of these files,
its runner, generated C++ sources, verified final artifact, generated native
library, runtime headers and runtime library. Before/after equality is required;
the generated adapter executable is checked after every case. Product hashes
are expected to differ between old/new implementations. Prefix hashes and test
source hashes must match for comparisons. Build commands include the native
compiler path. Compare only identical response latency, prefix, mode, cancellation
offset and worker selections. Physical memory addresses and all tensor inputs
are supplied by the host adapter identically to both products.

For example, from the repository root, run the following once with each product
root (each contains `product-cpp`, `product-build/libpycircuit_modules.a`, and
`product-final`):

```sh
LD_LIBRARY_PATH=/home/xiekunpeng/micromamba/envs/acir/lib \
  .pycircuit_out/venv312/bin/python \
  designs/sparsefa/tests/index_performance/run.py \
  --product /absolute/path/to/product-root \
  --toolchain /absolute/path/to/matching/runtime-install \
  --cxx /home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-g++ \
  --workers 1 2
```

Workers 1 and 2 execute the same DUT and cases. Their cycle metrics and checked
request/value behavior must agree; `workers` and `host_ms` naturally differ.
The bounded `--small-only --workers 2` baseline selection omits N512/N2048; do not
claim it measured those worker-two latencies. The full worker-one baseline
includes the complete N2048 computation without approximation. All full-matrix
candidate selections and any exclusions must be reported from their receipts.
