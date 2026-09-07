# Issue #48 IEX behavior closure

The in-tree I1, I2, and WBA H3 leaves now execute concrete generated-gfsim
behavior rather than stopping at source closure, frozen ACIR, C++ compilation,
or empty-state smoke. Every harness is generated from the current checkout and
compiled locally against this checkout's gfsim headers.

I1 covers exact read grant, denial/retry and reissue, stale and exact cancel,
same-cycle decision/cancel order, independent backpressure on all three outputs,
input retention, reset, isolated instances, and exact Queue push/pop counts. I2
covers both speculative operands, hit, miss, replay, wrong-producer ABA,
execute retry and acceptance, release, early/generated/active/post-accept
cancel, independent backpressure on all seven outputs, existing-tombstone
arbitration, tombstone capacity/reset, Flow isolation, instance isolation, and
exactly-once output ordering. WBA covers all four producer lanes,
VALUE/no-destination/fault/store/branch admission, producer contention, oldest
selection, independent backpressure on all four outputs, apply retry/success,
unpublished/apply-owned/completed cancel classification, drain, tombstone
suppression/reclaim, invalid input retention, reset, and instance isolation.

The first I2 token case exposed the generic state-guard SSA bug fixed first by
PR #70 at `187c9b81`: transaction guards now read the committed snapshot even
when the selected branch proposes a new value for the same state owner.

The module cards promote only the executed gfsim boundary. H2/H1 integration
and stateful PYC/RTL remain open roadmap work; the existing provisional-Table
PYC rejection stays explicit and is still covered by the WBA test.
