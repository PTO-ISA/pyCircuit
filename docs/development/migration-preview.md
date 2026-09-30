# From-source migration preview

`src/design_top.py` is the reusable hardware design. `counter.py` is compiled
as its own source unit and instantiated twice; `types.py` is a separate
declaration unit. The normal design has no `@system`, self receiver, stimulus,
checker, clock loop, or hand-written model runner.

The `hold_top.py`, `zero_rule_top.py`, and `failure_top.py` roots are focused
runner contract cases. They exercise registered hold, zero-rule quiescence,
and a source invariant failure after one committed step. `configs/` contains
finite canonical C3 configs. `oracle.py` is independent of the runner and owns
the expected values, epoch/status checks, and strict JSONL completion checks.

This is a source-tree preview fixture. It does not claim a released C ABI,
installed SDK, or production-compatible generated C++ class ABI.

## Build from this checkout

Run from the repository root. Prerequisites are CMake 3.25+, Ninja, Python 3.11+,
a C++20 compiler, LLVM/MLIR **22.1.8**, and Verilator with its CMake package.
The verified host is macOS arm64; these commands do not install a package.
For Homebrew LLVM 22:

```sh
cmake -S compiler/acir -B .pycircuit_out/m4-native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR="$(brew --prefix llvm@22)/lib/cmake/llvm" \
  -DMLIR_DIR="$(brew --prefix llvm@22)/lib/cmake/mlir"
cmake --build .pycircuit_out/m4-native --parallel 4 --target \
  acir-source-unit-harness acir-design-harness acir-cpp-source-parts-harness
cmake -S tests/integration/agentic-circuit/m4-preview \
  -B .pycircuit_out/m4-example -G Ninja \
  -DPYCIRCUIT_NATIVE_BUILD="$PWD/.pycircuit_out/m4-native"
cmake --build .pycircuit_out/m4-example --target preview-build --parallel 4
```

The outer Ninja graph runs public `pycircuit compile` separately for types,
counter and design_top, with header-only dependency inputs, then public
`pycircuit link` to `linked/design_top.ac`. A private helper reads a saved final
snapshot and emits both backends. Nested CMake projects independently compile
each C++ implementation source and link `pycircuit_system`; the declaration unit
has no fake `.cpp`. Verilator compiles the same hardware through a simulation-only
clock adapter. Both executables reuse the same SimExecutor and SystemRunner.

## Run and check

```sh
.pycircuit_out/m4-example/model-cpp/pycircuit_system \
  --config tests/integration/agentic-circuit/m4-preview/configs/three-ticks.json \
  --events .pycircuit_out/m4-example/cpp-events.jsonl
.pycircuit_out/m4-example/model-verilog/pycircuit_system \
  --config tests/integration/agentic-circuit/m4-preview/configs/three-ticks.json \
  --events .pycircuit_out/m4-example/rtl-events.jsonl
python3 tests/integration/agentic-circuit/m4-preview/oracle.py \
  .pycircuit_out/m4-example/cpp-events.jsonl \
  .pycircuit_out/m4-example/rtl-events.jsonl
```

The oracle checks the independent log sequence `100, 200, 0, 0, 3, 10`, per-instance
report gauges, and one `TERMINATED` Result at committed epoch `3`. The design has
six physical registers: four parent-owned registers plus one hidden register in
each child. Module connections alias the parent registers.

Use new event filenames for another run: existing files, links and nonregular
sinks are rejected before model build. `--events -` selects stdout. Omitting
`--events` executes silently, with success/failure conveyed by exit status.
The runner requires a finite positive canonical config limit; `{}` is not a
bounded run. It does not busy-loop a zero-rule design to reach a limit.

## Additional cases and output boundaries

Configure a separate build directory with `-DPYCIRCUIT_DESIGN=hold_top`,
`zero_rule_top`, or `failure_top` to select the checked-in focused designs.
`value_probe_top` and report-name probes support compiler validation. Expected
failure cases are checked by the external testbench, not relabeled as successful
models. Rebuild the same directory after source edits; native helper and Python
compiler dependencies are part of the graph. Concurrent independent builds must
use separate generated/build directories.

`artifacts/cpp` preserves source-owned groups; `artifacts/verilog` contains
aggregate hardware plus a simulation bridge. Their private `generated.json`
receipts protect file publication and do not certify code semantics or historical
producer identity. Invalid final IR, unsupported runner profiles and unmanaged
output replacements are refused. Damaged receipts, missing/extra files and unsafe filesystem nodes are refused;
receipt validation does not authenticate generated code contents.

This preview deliberately uses non-installed native helpers. Public new `emit`,
old-route retirement, complete C3 source maps, source-owned RTL
packaging and installed SDK remain later work. The old public `emit` route is
not used anywhere in this workflow. No model main, Work/Xfer traversal or fixture
algorithm must be hand-written by the user.

Acceptance and current evidence are tracked in the
[M4 completion work item](../work-items/m4-completion-workflow.md).
The [default-sink clarification](../rfcs/migration/approvals/m1-runner-default-sink.md)
records the user's explicit silent-default selection.

## M5 ABI prerequisite

The C++ preview now also builds `model-cpp/libpycircuit_dut.dylib` on macOS
(`libpycircuit_dut.so` on Linux) and generates `artifacts/cpp/dut.h`.
The header exposes the approved v1 model function table through Runtime headers;
its opaque handles use the same SimExecutor as the standalone runner. ABI
`configure_json("{}")` permits an unlimited session; the standalone runner still
requires a finite execution limit. No installed SDK or public emit cutover is
implied by this source-tree shared library.

Progress and remaining cutover tasks are tracked in the
[M5 work item](../work-items/m5-cutover.md).
