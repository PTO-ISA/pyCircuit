# Issue #63 OPT-05 aggregate Table observation borrowing

`SimTable::at` and `checkedAt` already return committed-storage references, but
generated C++ bound every `table_get` and nested field projection with `auto`,
creating source-level aggregate copies. The shared expression emitter now uses
lexical `const` references for aggregate Table observations and aggregate field
projections. Scalar observations remain values, and `checkedAt` keeps its
runtime range diagnostic.

ACIR remains value-only. `with` creates a writable copy, conditional and record
construction remain value expressions, and firing evaluation returns value
owned tuples. `StateTransitionPlan`, write proposals and Queue storage therefore
own complete values before the policy invocation ends. A generated executable
initializes one nested Entry, reads it by reference, replaces the same Table row
at commit, and consumes the queued old result in the following epoch; the sink
retains the old key and lane while the Table contains the replacement.

Both the flat CLI and hierarchy-preserving WBA paths check every frozen
aggregate `table_get` is emitted as a borrow, compile the full generated source,
and run the existing terminal/apply/cancel/drain/backpressure behavior matrix.
PYC/RTL continues to reject the provisional Table family under Decision 0155;
this C++ cost change does not claim new Table RTL support.

For the current WBA H3 source, nine aggregate `table_get` bindings and 371
nested aggregate projections change from by-value `auto` to lexical references.
The generated text grows by 2,660 bytes because the reference spelling is
longer. A same-source counterfactual that restores the exact previous bindings
and the new source both compile and execute with Apple clang `-O3`; Mach-O text
size is 212,992 bytes in both, while disassembled instruction counts are 44,704
and 44,626. These diagnostics show no measured runtime speedup and are not
semantic pass criteria.
