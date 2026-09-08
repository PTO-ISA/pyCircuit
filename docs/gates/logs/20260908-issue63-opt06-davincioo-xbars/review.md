# Issue #63 OPT-06 DavinciOO XBAR closure

The design PR adds the three registered TMU BGF, GPE IPF, and MEM NOC H3 XBAR
pilots after the Decision 0227 framework prerequisite merged. Each design keeps
its frozen nominal packet schema, four ingress Queues, two round-robin merges,
four-way route, Queue depths, base latencies, per-destination latency tuple, and
delivery-only marker while replacing four handwritten `apply` calls with one
static `ac.array` template.

Review-driven regressions freeze each packet field and width independently of
the implementation, seed and compare every preserved field, inspect exact ACIR
merge/route/transform parameters, exercise two packets from one ingress under
cross-ingress contention, hold the sink under backpressure, reset an in-flight
instance while an active peer continues, and inject fresh traffic after reset.
PYC C++/Verilog build checks are separate tests and report an explicit skip when
the required local toolchain is absent. Python-only CI still verifies all three
source schemas and exact ACIR topologies without invoking native generators.

The catalog checker now validates the closed set of execution statuses and the
matching contributor checklist marker. All three rows are recorded as
implemented and verified; H2/H1 integration and module-specific NDF L0/L1 links
remain explicitly pending in their cards.

Independent review found no unresolved P0, P1, or P2 issues after these changes
and recommended approval.
