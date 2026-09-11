# Indexed same-owner branch-join gate

Decision 0209 joins both branch index and branch value before one persistent
list proposal.

## Evidence

- Both source indices retain exact-width and constant/full-domain safety checks.
- The frontend emits two typed `ac.var.select` operations followed by one
  unconditional `ac.var.assign_element`.
- Storage selection and QueueGraph preserve one dynamic Table write.
- Generated gfsim computes two ternaries and prepares only the joined
  index/value pair; the alternate entry is not written.
- Native execution observes entry1=8 on the false arm and entry3=9 on the true
  arm while preserving the previously written entry.

## Gates

- Focused frontend join tests: 3/3 passed.
- Focused indexed/scalar integration: 2/2 passed.
- ACIR lit: 165/165 passed.
- Native C++ suites: 15/15 passed, including CodeGen 105/105.
- Python frontend/public API: 97/97 passed.
- Queue integration: 26 passed, 1 optional skipped.
- Agentic Circuit repository contracts: passed.
- ROB counters remain 1769/182/215/511; ISQ remains 1377/200/162/238.

## Remaining boundary

The first optional selected output is closed by Decision 0210. Multiple
outputs, nested/multi-block CFG, cross-owner value joins, multi-input branches,
and combined blocking/discard paths remain open.
