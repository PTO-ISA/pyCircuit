# Nested specialization activation gate summary

Decision 0192 recursively binds specialization-local activation evidence to
placement-specific dense ObjectIds without flattening reusable module classes.

## Evidence

- The resolver uses one deterministic local layout: internal Queues, blocks,
  Tables, and then child ObjectId intervals.
- Child interface names bind to resolved parent Queues; nested activation edges
  are merged into the root CSR arrays.
- Direct and nested Python module integrations run scan and incremental modes
  and produce identical `6/11` results.
- The mixed local-transform/internal-Queue/child specialization reports
  `activation_complete() == true`, retains one internal Queue per parent
  instance, and preserves one class per specialization.
- Focused nested and mixed QueueGraph CodeGen tests: 2/2 passed.
- Focused direct/nested Python module integration: 2/2 passed.
- Full CodeGen suite: 105/105 passed.
- Python frontend: 75/75 passed.
- Full Queue codegen integration: 20 passed, 1 skipped because the optional
  DavinciOO reference trace fixture was unavailable.
- C++/Python format, repository contracts, strict 192-row decision status,
  MkDocs strict, and `git diff --check`: passed.

## Remaining scope

Activation can now bind all hierarchy shapes currently admitted by structured
QueueGraph codegen. The remaining work is to generalize the underlying internal
graph shapes, separate Work from Xfer closure scheduling, add output-dequeue
adapters, and complete full ROB tick/timeline equivalence.

Decision 0193 subsequently separates Work and Xfer closure scheduling and moves
external input offers to an explicit resource-Xfer frontier.
