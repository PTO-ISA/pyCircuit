# Reusable oldest-ready ISQ gate summary

Decision 0197 adds storage-neutral collection selection and uses it in a real
reusable issue queue without exposing hardware state or handshake objects in
Python.

## Evidence

- `ac.find` queries an ordinary persistent list and returns
  `valid/index/value`.
- Raw frontend output contains `ac.var.match/choose/read_element`, never an
  authored Table.
- Storage selection preserves predicate/key regions and lowers the query to
  existing Table semantics with verifier-checked provenance and widths.
- A predicate may read a second persistent list. The read-only owner becomes an
  activation source but not a transaction resource.
- Generated policies capture read-only state through const pointers and scan
  one choose operation once; writable owners alone join atomic commit.
- The reusable ISQ contains first-free dispatch, 64 persistent readiness bits,
  minimum-age issue, and two placements backed by one generated class.
- Runtime coverage includes readiness before dispatch, readiness plus dispatch
  in the same epoch, resident wakeup, oldest-ready order, a retained fifth
  request, output backpressure without early state clearing, independent
  instances, and false/true readiness across tag reuse.
- Validation passes: ACIR lit 160/160, CodeGen 105/105, gfsim 259/259,
  frontend/public API 90/90, and Queue integration 23 passed with one optional
  external DavinciOO fixture skipped.
- A separate flat QueueGraph stdin pipeline lowers the captured read-only list
  query and passes C++20 syntax compilation, matching the structured module
  path used by the reusable ISQ.

## Remaining scope

Add the same per-tick scan/incremental state and commit-timeline comparison used
by the ROB. General typed CFG path predicates, optional output presence, and
explicit conflict-class/arbitration IR remain open before broader conditional
transaction shapes are complete.
