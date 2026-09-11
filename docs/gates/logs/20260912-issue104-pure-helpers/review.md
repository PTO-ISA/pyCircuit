# Issue #104 typed pure-helper closure

ACPy now accepts top-level helpers with exact closed parameter and result types
inside rules and Queue expressions. Ordinary functions remain private
`func.func` definitions with `func.call` sites; `@ac.inline` marks mandatory
compiler expansion. The source subset covers local rebinding, bounded
conditionals, fixed tuple results and direct unpacking, and nested pure calls.
It rejects recursion, loops, early returns, path-undefined or captured values,
Queue/Table/state effects, I/O, allocation, unknown calls, and signature
mismatches.

The ACIR pass validates the complete Queue/rule helper closure and expands only
mandatory-inline calls before QueueGraph planning. QueueGraph retains ordinary
helper definitions and call identities. GFSim emits typed C++ helper functions;
PYC legalization recursively expands every remaining helper call because PYC
has no call operation.

The parity fixture runs the same boundary values through generated GFSim and
PYC C++ and compares packed outputs. It includes `u8` wraparound at 255 and
proves ordinary and forced-inline results agree without adding Queue, state,
rule, or commit boundaries.

Adversarial review found and closed three gaps before merge: dead expressions
now undergo purity validation before elimination, pure and stateful structured
modules share the helper registry without duplicate definitions, and native
helper legality uses an explicit pure-operation allowlist that rejects state
reads and writes at the defining boundary. The final re-review reported no
remaining P0-P2 findings.
