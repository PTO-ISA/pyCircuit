# Parent-owned State interfaces

`Top` owns five States. A separately compiled external Rule computes
`incremented = left + 1`. Two instances of `Stage` read different parent States
and write existing parent output States. Each Stage owns one `accepted` counter;
its nested Rule takes a typed input and returns a typed result. `Export` returns
the first output's existing identity. It allocates no storage.

The Stage Rule increments its internal counter and returns `value + accepted`
while the old counter is below two. All reads use committed old Q. The result
and counter update commit together; after two updates both hold. The independent
four-cycle trace, sampled before each rising-edge commit, is:

| Cycle | Incremented | First output | Second output | Each accepted counter |
| --- | --- | --- | --- | --- |
| 0 | 0 | 20 | 30 | 0 |
| 1 | 4 | 0 | 10 | 1 |
| 2 | 4 | 5 | 11 | 2 |
| 3 | 4 | 5 | 11 | 2 |

The alias always equals the first output. The compiler test hides provider bodies
while compiling the consumer, removes all source files before linking, checks
storage ownership, and compares generated C++ workers 1/2 and RTL to this oracle.
A test adapter also drives reset after the blocked cycle and checks that all
parent and child States return to their initial values and resume the same trace.
For Queue timing and backpressure, see [scalar Queue pipeline](../scalar_queue_pipeline/README.md).

With an installed toolchain:

```sh
pycircuit run examples/parent_state_interfaces --cycles 4
pycircuit run examples/parent_state_interfaces --target verilog --cycles 4
```

The shared example verifier checks the per-cycle trace in [expected.json](expected.json),
including both low and high sampling epochs, against these independent expectations.
