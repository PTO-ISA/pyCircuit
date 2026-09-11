# Reusable ISQ tick-equivalence gate summary

Decision 0198 proves the complete reusable oldest-ready ISQ scenario at every
committed boundary rather than comparing final outputs only.

## Evidence

- The same generated dual-instance model runs once with every dispatch row
  scanned and once with generated incremental activation/Work-closure CSR.
- Every step compares six boundary Queue images, eight resident entries, 128
  readiness bits, exact epoch, and the validated commit timeline.
- The scenario covers readiness before dispatch, readiness and dispatch in one
  epoch, resident wakeup, oldest-ready order, full input retention, result
  backpressure, instance isolation, and tag readiness clear/reuse.
- Exact counters are frozen by the executable test: 952 scan Work calls versus
  129 incremental Work calls, 100 activation traversals, and 146 Work-closure
  traversals.

## Remaining scope

The functional ROB and ISQ plus their incremental-equivalence gates are now
present. General typed CFG path predicates, selected output-presence sets,
read/write conflict classes, and explicit arbitration IR remain open before
the broader rule-lowering flow can be called complete. Full AC G0/G1/G2 and
pyCircuit semantic closure are still required afterward.
