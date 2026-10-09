# Historical expectation and credit systems

These API-owned fixtures retain the algorithms from
`examples/agentic-circuit/pipelines/gfsim_expect_pipeline.py` and
`pyc_credit_pipeline.py` at baseline
`8887e6dec7b4cc530a9967c860dc6a224d79a4ab` through the current source-unit route.
Run `Source/source-historical-pipelines.test` with the existing compiler lit gate.
Each source compiles independently; explicit linking produces the common final
artifact used for C++ and Verilog emission.

The expectation DUT retains the original `ExpectedToken.value: u16`, depth-two,
latency-one queue and pure `value > 0` predicate. Checking depends on head
availability and is independent of sink readiness. Its positive system offers
64 boundary values, fills and holds the queue, then drains in 70 cycles. Separate
systems cover an invalid external zero and a zero head with the sink blocked.
The host checker derives queue behavior from a bounded FIFO; it never supplies
DUT values. Historical `expectation_failed` becomes the current generic
`source_check_failed`, with the original `value must be positive` message. The
pure predicate is checked on every available view; the old suppression marker
for repeated views of one held token has no observable effect on this predicate.

The credit systems import `CreditPipeline` and `CreditToken` from the existing
`queue-source/pyc_credit_pipeline.py` provider. They retain two credit slots and
the original depth-four, latency-one input and completion queues. Full independent
C1, C4, C5, C7 and C8 histories run for 20, 26, 32, 20 and 44 cycles respectively.
These cover unequal-cost completion order, completion backpressure, input
saturation, width and cost boundaries, and held credit during output blockage.
The existing independent credit oracle compares every low/high old-Q result;
each history starts as a fresh system. Expected results stay outside DUT source.

Zero-cost and deferred zero-cost credit faults, sibling rollback, physical reset,
and sticky-failure recovery remain owned by `queue-source-faults.test` and the
independent queue-source oracle gate. Fresh-system construction here does not
establish physical-reset recovery. The lit test validates C++ workers one and
two and Verilator; it does not claim four-state coverage or a full nightly run.
