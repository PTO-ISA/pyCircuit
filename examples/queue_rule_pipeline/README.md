# Queue and Rule pipeline

This small complete System combines typed Module hierarchy, cross-module Rule
calls, a finite loop, persistent state, Queue admission, local EHR ordering and
Rule output availability through the existing compile/link/emit flow.

`QueueRulePipeline` contains two depth-one, latency-one queues and an
`Accumulator` child. A `Packet` groups its tag and value into one interface.
The transfer Rule expresses the entire processing step:

```python
@ac.rule
def transfer() -> None:
    packet = ingress.pop()
    result = accumulator.add(packet)
    transferred = transferred + 1
    egress.push(result)
```

The child adds two to the input using a two-iteration combinational loop, then
adds that value to its persistent total. Its local `value` is a wire; `total`
and the parent's `transferred` become storage. No loop iteration adds a cycle.

If the output queue cannot accept the result, **all** transfer proposals are discarded: the
input head, child total and parent count stay unchanged. Authors write no
ready/valid tests. The local `progress` Rule explicitly runs after `transfer`
and sees its accepted candidate count in the same cycle; an independent
snapshot sees committed state. This demonstrates local ordered visibility,
not arbitrary ordering between Rules declared in different modules.

The bench sends four packets with values 10–13 and pauses consumption until
cycle 6. `consume()` returns a Packet to a separate checking Rule. The compiler
suppresses that consumer when the producer result is unavailable; there is no
authored validity field, zero substitute or holding register.

| Cycles | Expected behavior |
| --- | --- |
| 0–1 | Accept two inputs; transfer the first into the output queue. |
| 2–5 | Full output without a simultaneous pop blocks transfer and input acceptance; total stays 12, count stays 1. |
| 6–9 | Drain tags 0, 1, 2, 3 with totals 12, 25, 39, 54. |
| 10–11 | Both queues are empty; counts and total hold. |

`bench.py` checks packet order, payloads, blocked state and final completion.
The independent regression in
[`test_queue_rule_pipeline_example.py`](../../tests/system/test_queue_rule_pipeline_example.py)
also checks every sampling epoch, same-cycle candidate visibility, exact output
publication and interface-only consumer compilation.

## Run

```sh
pycircuit run examples/queue_rule_pipeline --target cpp --cycles 12
pycircuit run examples/queue_rule_pipeline --target verilog --cycles 12
```

For the existing C++ workers 1/2 and RTL comparison:

```sh
cmake -S examples/queue_rule_pipeline -B /absolute/build/queue_rule_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/queue_rule_pipeline --parallel 2
ctest --test-dir /absolute/build/queue_rule_pipeline --output-on-failure --no-tests=error
```

The source files compile independently; both targets consume the same linked
artifact. This twelve-cycle example uses known inputs and the regular System
clock/reset harness. Existing API regressions own X/Z, explicit Discard/retry
and failing-check rollback coverage.

This is the minimal combined flow for issue #319, not completion of that issue.
Queue now uses ordinary native FIFO instances and Method calls internally.
Cross-hierarchy partial/EHR composition, DFF/DFFE selection and the remaining
large Systems are separate pending work. No compiler limits or existing example
stimuli are changed.
