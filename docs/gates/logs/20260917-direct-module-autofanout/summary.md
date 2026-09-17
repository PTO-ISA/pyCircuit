# Direct-module automatic fanout evidence

## Contract

This change closes the direct `@ac.module` system-composition case covered by
Decision 0185. It implements the existing Agentic Circuit specification under
“Multiple consuming uses”: destructive Queue values with more than one
consuming use receive one compiler-inserted strict `ac.broadcast` at their
lexical lowest common parent. The broadcast consumes its input only when every
output can accept the token and retains no hidden per-output progress.

No public Python syntax or semantic contract changed. Decision 0189's shared
module body lowering, Frozen ACIR verification, QueueGraph specialization, and
generated gfsim execution remain the enforcement path.

## Root cause and implementation

The ordinary Queue/rule frontend already inferred lexical-LCA broadcasts. The
independent direct-module system assembler instead counted uses and rejected
every Queue value whose use count was not exactly one with `ACPY-MODULE-002`.
Consequently, one typed system input could not feed two ordinary module calls.

The frontend now records every module operand and system-return consumption,
inserts one compiler-owned broadcast specialization at the Top/LCA, and remaps
each consuming occurrence to one output. The synthetic specialization contains
the canonical `ac.broadcast`; Top contains only legal structural instances.
Producer and use locations are retained as fused source provenance, concrete
payload types remain specialization inputs, and generated C++ uses
`gfsim::QueueBroadcast<T, N>`.

QueueGraph C++ generation now accepts the corresponding one-input,
two-or-more-output direct-interface broadcast specialization. It constructs one
`QueueBroadcast`, preserves the generated activation graph, and adds no
consumer-specific fanout state or partial-progress path.

## Provenance

- Previous branch head: `f69a0861b8ecdd74732a4014385c08526f2dbcfc`.
- Implementation commit: `af623bc78cb52fa64f7ac55c2d34492e24b35866`.
- Exact record-projection follow-up: `75ad4584`.
- Branch: `codex/fix-module-config-metadata`.
- Consumer-neutral reproducer: `/tmp/ssm_memory_fanout_repro.py`.

## Focused verification

All commands ran from the implementation commit's pyCircuit checkout.

```text
PYTHONPATH=python/semantic-core/src:python/agentic-circuit/src \
  python3 -m unittest \
  tests.python.agentic-circuit.python_frontend.test_queue_frontend -q
```

Result: 221 tests passed.

```text
PYTHONPATH=python/semantic-core/src:python/agentic-circuit/src \
  python3 -m unittest \
  tests.python.agentic-circuit.python_frontend.test_queue_frontend.QueueFrontendTest.test_repeated_direct_module_input_inserts_strict_atomic_broadcast \
  tests.python.agentic-circuit.python_frontend.test_queue_frontend.QueueFrontendTest.test_typed_module_calls_lower_to_structured_reusable_acir \
  tests.python.agentic-circuit.python_frontend.test_queue_frontend.QueueFrontendTest.test_host_result_mode_preserves_root_queue_returns \
  tests.python.agentic-circuit.python_frontend.test_queue_frontend.QueueFrontendTest.test_nested_module_call_preserves_parent_child_structure -v
```

Result: 4 tests passed.

```text
PYTHONPATH=python/semantic-core/src:python/agentic-circuit/src \
  python3 -m unittest \
  tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_direct_module_fanout_freezes_generates_and_runs_atomically \
  tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_typed_module_calls_generate_one_reusable_class_and_run \
  tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_nested_python_module_calls_preserve_reuse_and_run -v
```

Result: 3 tests passed. The new case passed raw frontend lowering, Frozen ACIR
verification, QueueGraph planning, C++ generation, C++20 compilation, and
runtime execution. Input `5` produced exactly one `6` and one `10` through the
two independently instantiated consumers.

```text
cmake --build .pycircuit_out/toolchain/build \
  --target acir-queue-cxxgen -j4
```

Result: the modified native QueueGraph generator rebuilt successfully.

The external reproducer also passed `ac-queue-cxxgen.py` with explicit
`acir-opt`, `acir-queue-plan`, and `acir-queue-cxxgen` tools. Its Frozen ACIR
contains one `ac.broadcast` with fused producer/use provenance; its plan
contains one broadcast specialization and three structural instances; its
generated source contains `gfsim::QueueBroadcast<gfsim::UInt<1>, 2>` and passes
`c++ -std=c++20 -fsyntax-only`.

Formatting hooks and `git diff --check` passed for the implementation diff.

## Exact record-projection follow-up

The complex consumer exposed a separate operand-normalization gap after the
original fanout fix: direct module placement accepted only bare Queue names, so
an ordinary typed call such as `consume(packet.header.valid)` fell through to
the generic unsupported-`Assign` diagnostic. This did not reclassify or damage
stateful rule modules; a reduced stateful module with one duplicated bare input
already passed at `90a70d46`.

The frontend now materializes an exact attribute chain rooted in a named Queue
as a compiler-owned pure projection specialization. Each projection remains a
real destructive consuming use of its root Queue, so the same lexical-LCA
fanout analysis inserts one strict broadcast before two or more projections or
direct consumers. Unsupported arbitrary expressions still fail closed.

Focused evidence after commit `75ad4584`:

```text
PYTHONPATH=python/semantic-core/src:python/agentic-circuit/src \
  python3 -m unittest \
  tests.python.agentic-circuit.python_frontend.test_queue_frontend -q
```

Result: 222 tests passed.

```text
PYTHONPATH=python/semantic-core/src:python/agentic-circuit/src \
  python3 -m unittest \
  tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_direct_module_fanout_freezes_generates_and_runs_atomically \
  tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_record_projection_operands_freeze_plan_and_generate_cpp \
  tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_typed_module_calls_generate_one_reusable_class_and_run \
  tests.integration.agentic-circuit.e2e.test_queue_codegen.QueueCodegenTest.test_nested_python_module_calls_preserve_reuse_and_run -q
```

Result: 4 tests passed. The new stateful case preserves its inferred Table
owner behind the broadcast and executes generated C++. The projection case
freezes two typed `ac.var.get` transforms, records five structural instances in
QueueGraph, emits `QueueBroadcast<Packet, 2>`, and passes C++20 syntax
compilation.

The consumer Core then advanced past every direct module record-projection
operand. Its next error is independently reduced to a nested-rule payload in
the consumer's `memory_ingress_join`; that issue is outside this fanout and
projection evidence slice.
