# Scalar Queue pipeline

`BiasStage` adds one to each eight-bit input. `SumStage` calls the Method of an
existing `Accumulator` and publishes the explicitly computed next total. Inputs
3, 5, 2, 7 therefore produce 4, 10, 13, 21, in order.

The connecting `Top` owns default depth-2 input/output Queues and an explicit
depth-3 Queue between the stages. Consumers use typed scalar
parameters; they do not repeat empty/full checks. The consumer waits ten cycles,
so a full output Queue holds the input, SumStage's accepted count and the
Accumulator's total together. Queue admission uses occupancy before the edge:
an empty Queue cannot forward a concurrent push, and a full Queue cannot borrow
capacity from a concurrent pop.

The CodeGen `transactional-queue-pipeline.test` checks separate compilation using
provider interfaces alone, both generated targets, an independent old-occupancy
model, and explicit depth 3. It compares every observation epoch, including
blocked cycles; the expected sequence comes from the arithmetic above.

`queue_rule_pipeline` remains the historical structured-payload example. This
scalar example does not establish migration of its Struct-specific behavior.

With an installed toolchain:

```sh
pycircuit run examples/scalar_queue_pipeline
pycircuit run examples/scalar_queue_pipeline --target verilog
```
