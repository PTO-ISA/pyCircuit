# Reusable ROB tick-equivalence gate summary

Decision 0196 replaces final-result-only confidence with a full committed-state
comparison between scan and incremental activation.

## Evidence

- Two independent instances of the same generated dual-ROB model run under a
  full dispatch-row scan and the generated activation/Work-closure plans.
- Every tick compares ten Queue contents, head/tail/count/epoch for both ROB
  placements, all eight entries, and the ordered validated commit timeline.
- The scenario covers allocation and retirement output backpressure, full and
  empty state, a retained fifth request, out-of-order completion and in-order
  retirement, wrap, stale generation, recovery epoch, and old-epoch rejection.
- The second ROB placement allocates, completes, and retires independently while
  the first placement executes the full scenario.
- Exact counters are locked by the test: 1769 full-scan Work calls versus 182
  incremental Work calls, 215 activation traversals, and 545 Work-closure
  traversals.
- Decision 0202 later removed the conservative epoch self-write. The current
  equivalent scenario retains 1769/182 Work and 215 activation traversals while
  reducing Work-closure traversals to 511.
- The complete Queue integration lane passes 22 tests; the optional external
  DavinciOO fixture gate is skipped when its checkout is unavailable.

## Remaining scope

The next critical path is compiler-recognized selection and bulk update over a
normal persistent Python list, followed by a reusable oldest-ready ISQ with
persistent ready lookup, lost-wakeup coverage, result backpressure, and two
independent placements. Typed path/output-presence/conflict IR remains required
for general conditional transactions beyond the currently verified ROB shape.
