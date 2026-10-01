# Current M5 profile index

The implementation branch has accepted M5 for the scalar source-unit profile.
This planning checkout retains historical source and contract documents; use the
[pinned implementation migration guide](https://github.com/PTO-ISA/pyCircuit/blob/5b0d610d/docs/development/m5-migration.md)
for the actual current workflow and supported source forms.

The accepted route is per-source Python capture, MLIR-owned semantic analysis,
explicit unit link, then CPP/Verilog emit from the same saved final design.
The bounded authoring profile is portless function modules, lexical registered
rules, finite scalar state, one default clock and empty static arguments.
Runtime-only and CompilerDev consumers use the unified runtime; there is no
legacy builder/JIT/QueueGraph fallback. This index widens no capability.

M3/M6 expansion and carrier approval prerequisites are in the
[roadmap](../work-items/m3-m6-expansion-plan.md). Historical native/capture cases
cannot be substituted for public-profile end-to-end acceptance.
