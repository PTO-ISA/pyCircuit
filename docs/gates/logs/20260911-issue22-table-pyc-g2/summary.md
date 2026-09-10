# Issue 22 AC G2 result

The complete AC G2 matrix, including the new bounded Table PYC C++/Verilator
parity and the existing direct/native gfsim Table suite, passed from the
current integrated toolchain. Root unit and API-hygiene checks also passed.

The wrapper exited only at the final release-wide strict decision-status gate:
Decisions 0232 and 0234 still require real Linux/macOS candidate workflow
evidence. That platform release blocker is independent of Decision 0241 and is
retained for the release phase rather than relabeled as a Table failure.
