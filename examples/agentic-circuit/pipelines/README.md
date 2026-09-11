# Queue and value pipelines

The [Agentic Circuit Specification Manual](../../../docs/acir/spec/agentic-circuit.md)
defines the authoring, ACIR, runtime, backend, and refinement contracts exercised
by these examples.

The payload examples cover exact bit widths, immutable nominal structs,
standard Python enums, fixed arrays, nested aggregates, and masked matching.
They prove that layout and value semantics stay consistent across ACIR, gfsim,
PYC C++, and Verilog without embedding a consumer instruction format.

The routed dependency pipeline is a vendor-neutral, architecture-scale
QueueGraph topology. It combines dependency scheduling, four-way routing,
round-robin merge, ordered output, observations, and backpressure using only
framework primitives:

    source -> transform -> dependency window -> 4-way route
                                                 | route 0
                                                 | route 1
                                                 | route 2
                                                 | route 3
                                            merge -> reorder -> output -> sink

The regression for this example checks topology cardinality, deterministic
generation from copied source, and generated C++ compilation. It does not load
a product trace or compare against a processor reference model.

## PYC and Verilog slice

`pyc_queue_pipeline.py` exercises the initial scalar hardware lowering. Generate
frozen ACIR first, then run `acir-queue-pycgen` or the bundled
`compiler/acir/tools/ac-queue-pyc-build.py` command. The bundle command
validates the pinned toolchain lock, invokes external `pycc` for C++ and
Verilog, compiles the C++ source, runs Verilator lint, and writes a canonical
hash manifest.

The repo-local pyCircuit 6 toolchain contract is recorded in
`toolchains/agentic-circuit/pyc.lock.json`. Build it with the repository's
pinned LLVM 22 toolchain before running the PYC gate.

`pyc_struct_pipeline.py` verifies deterministic packed struct layout.
`pyc_route_merge_pipeline.py` verifies static selector demux and priority merge
logic, including forward valid and backward ready paths.
`pyc_select_pipeline.py` verifies runtime selection from a statically shaped
Queue collection without dynamic Queue pointers.
`pyc_rule_pair_pipeline.py` verifies two independent rules without introducing
an implicit global transaction or priority.
`pyc_multi_input_rule_pipeline.py` verifies that one serial `@ac.rule` consumes
two Queue tokens and produces one result atomically, while MLIR supplies the
input-availability, output-backpressure, and commit-group mechanics.
`inferred_boundary_pipeline.py` verifies that ordinary typed system parameters
and returns are enough for MLIR to create the source/sink boundaries.
`inferred_module_pipeline.py` defines a pure typed `@ac.module` and invokes it
twice with ordinary Python calls; MLIR creates the structured instances and
gfsim emits one reusable specialization class.
`inferred_nested_module_pipeline.py` returns one module call from another module
and proves the child and wrapper classes are each emitted once.
`pyc_rule_pipeline.py` verifies that the simple `@ac.rule` surface lowers
through typed obligations and internal firing IR to the standard transform.
`gfsim_expect_pipeline.py` verifies a verification-role leaf executes in gfsim
and is rejected from PYC design hierarchy with testbench-boundary guidance.
`pyc_fork_pipeline.py` verifies decoupled fanout with per-output delivered state.
`pyc_conditional_pipeline.py` verifies that a serial runtime `if` becomes an
official route, two branch transforms, and a mutually exclusive priority merge.
`pyc_feedback_pipeline.py` verifies a bounded serial `while` as sequential
feedback data, valid, and iteration state shared by PYC C++ and Verilog.
`pyc_loop_control_pipeline.py` verifies a leading runtime `break` and tail
`continue` normalize to explicit bounded feedback conditions.
`pyc_recursive_pipeline.py` verifies bounded compile-time recursion expands to
a frozen three-stage Queue chain before ACIR publication.
`pyc_reorder_pipeline.py` verifies bounded key-ordered retirement with the same
register-bank and handshake semantics in typed gfsim, PYC C++, and Verilog.
`pyc_dependency_pipeline.py` verifies predecessor wakeup, execution countdown,
out-of-order completion, and PYC C++/Verilator cycle equivalence.
`persistent_schedule.py` verifies that high-level `ac.schedule` keeps its provider
identity through Frozen ACIR and QueueGraph, retains bounded completion after a
producer leaves the output window, and generates the gfsim v2 specialization.
`pyc_barrier_pipeline.py` verifies heterogeneous positional payloads and an
all-input/all-output atomic synchronization firing shared by typed gfsim, PYC
C++, and Verilog.
`pyc_credit_pipeline.py` verifies bounded parallel in-flight slots, independent
cost countdown, deterministic completion selection, automatic credit return,
and PYC C++/Verilator cycle equivalence.
`pyc_memory_pipeline.py` verifies an explicit typed memory instance, old-data
read-during-write behavior, aligned request/response state, and exactly one
`pyc.sync_mem` realization per instance in PYC C++ and Verilog.
