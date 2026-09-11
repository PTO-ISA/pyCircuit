# Table semantic activation gate summary

Decision 0195 separates a committed transaction from an observable committed
value change. Equal final Table values complete the same atomic Probe/Commit
group but no longer propagate activation to Table subscribers.

## Evidence

- `SimTable` compares only touched equality-comparable entries before and after
  the complete deterministic merge/replace sequence.
- Non-comparable entry types retain conservative wake behavior.
- `SimSystem` keeps commit ticks, progress, and observations tied to commits;
  only activation-source collection uses semantic change.
- The focused runtime regression proves equal replacement commits and consumes
  its input with zero activation traversals, while a changed replacement wakes
  exactly one declared subscriber.
- The full gfsim suite passes 259/259 tests.
- The reusable circular ROB integration continues to compile and execute with
  one shared specialization class and independent instance state.

## Remaining scope

The reusable ROB still needs full scan/incremental equivalence for every tick,
Queue, lexical state owner, output time, and commit timeline. The Pythonic
oldest-ready ISQ and typed path/conflict analysis remain open.
