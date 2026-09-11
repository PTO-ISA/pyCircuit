# Issues #98 and #99 API hygiene closure

The pyCircuit package keeps `probe` and `testbench` as modules and exposes the
matching decorators only through `pycircuit.design`. Signal, Wire, and
CycleAwareSignal priority encoding now share one generic result type. Static
API hygiene and JIT use one declarative receiver policy: CAS keeps the five
normative conversion/selection helpers, while named comparison methods are
rejected for every receiver in favor of operators.

Agentic Circuit now publishes an exact runtime `__all__` and a separate
29-name capture-only marker inventory under `agentic_circuit.markers`.
Established root-qualified and explicit root imports remain capture-compatible,
but ordinary Python calls fail with capture-time-only guidance and never return
runtime placeholders.

The public width signatures and shared pyCircuit exception hierarchy were
already present on the base revision; this change adds regression coverage so
all five findings in issue #98 remain closed together.
