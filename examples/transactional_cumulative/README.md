# Transactional cumulative pipeline

`design.py` preserves issue #361's cumulative pipeline: eight-bit input samples
are incremented, then a sibling Accumulator Method adds them to a sixteen-bit
sum. Ordinary Rules register explicitly. The Method admits at most four samples;
input consumption, both owners' updates and output insertion commit together.

`Top.py` sends 3, 5, 2, 7, 3 and delays output consumption until cycle 10.
The first four results are 4, 10, 13, 21. The fifth biased sample remains in the
intermediate Queue after the Method guard closes. Full output occupancy holds
the complete accumulation transaction, even when a consumer pops that cycle.
The queues use the default depth of two.

The checked-in `expected.json` records independent cycle expectations for the
System's send, receive and progress observations. The CodeGen
`transactional-issue361.test` additionally observes both state owners and the
intermediate head, checking every epoch against an old-occupancy model. It
compiles the design and System separately and resumes a saved Linked artifact.

```sh
pycircuit run examples/transactional_cumulative
pycircuit run examples/transactional_cumulative --target verilog
```
