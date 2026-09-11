# Issue 97 shared-contract convergence evidence

Agentic Python now has one strict I-JSON validator and one product contract
epoch. Package metadata and schemas are checked mirrors. Canonical JSON remains
the hash representation, while a distinct MLIR byte escaper round-trips quotes,
backslashes, control bytes, and Unicode through the native parser.

The implementation-neutral semantic primitive registry is validated at build
time and generates the complete contract table consumed by PYC verification
and Verilog-only selection. It covers semantic IDs, enums, width formulas,
dependencies, and zero-input behavior without carrying implementation names.
PYC, ACIR, gfsim, structural Python, and Agentic Python use one helper per
language boundary; exhaustive tests admit widths 1..64 and reject 65..130.

All focused Python, PYC, ACIR, gfsim, compatibility, inventory, documentation,
and repository gates pass from the current checkout. See `commands.txt`, the
adjacent bounded outputs, `summary.json`, and `decision_status_report.json`.
