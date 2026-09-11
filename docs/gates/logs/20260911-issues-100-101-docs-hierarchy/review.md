# Issues #100 and #101 consistency closure

Current-main audit separated already-fixed findings from active gaps. The active
documentation tree now has audience-based sections and a navigation completeness
test; all moved links build strictly. Gate evidence requirements describe the
actual focused, decision-bearing, scripted, and release profiles rather than
requiring every historical run to contain every possible artifact.

Engineering cleanup moves `pyc-opt` to the explicit MLIR CLI/config driver and
links transform objects so static pass registrations cannot be discarded.
Makefile smoke stages the install tree before consuming it. Agentic commands use
one shared exit-code enum, unreachable commands no longer publish placeholder
success, source resources no longer assume `parents[4]`, and Queue/rule capture
preserves diagnostics.

Agentic frontend goldens are centralized in their declared directory. The
Dodgeball example now runs in required unit CI. One ACPy-authored primitive
pipeline compares priority encode, popcount, and both zero-count directions in
typed gfsim and PYC C++ over zero, all-one, one-hot, and multi-hot inputs.
