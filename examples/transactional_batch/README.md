# Transactional four-sample batches

`design.py` preserves issue #361's separate batch example. Module combinational
values feed Method guards and Value Methods. The controller's Module `if/else`
selects collection or emission from committed state; the branch-defined output
resource remains available for export by `return out`.

`Top.py` sends three batches of 3, 5, 2, 7. Each produces 17. Consumption starts
at cycle 24, so the depth-two output Queue fills and the third complete batch
must remain intact until emission can commit. A blocked collect cannot select
the else arm. A successful emit clears both owners together; collection resumes
the following cycle.

The checked-in `expected.json` comes from an independent old-occupancy cycle
model. The CodeGen `transactional-issue361.test` additionally observes the total,
sample count and controller count at every epoch, including output stalls.
It compiles design and System separately and resumes a saved Linked artifact.

```sh
pycircuit run examples/transactional_batch
pycircuit run examples/transactional_batch --target verilog
```
